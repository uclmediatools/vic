/*
 * FILE:    mbus.c
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1997-99 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define MBUS_ENCRYPT_BY_DEFAULT

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "net_udp.h"
#include "hmac.h"
#include "base64.h"
#include "crypt_random.h"
#include "qfDES.h"
#include "mbus.h"

#define MBUS_BUF_SIZE	  1500
#define MBUS_ACK_BUF_SIZE 1500
#define MBUS_MAX_ADDR	    10
#define MBUS_MAX_PD	    10
#define MBUS_MAX_QLEN	    50 /* Number of messages we can queue with mbus_qmsg() */

#ifdef NEED_VSNPRINTF
int vsnprintf(char *s, int buf_size, const char *format, va_list ap)
{
	/* Quick hack replacement for vsnprintf... note that this */
	/* doesm't check for buffer overflows, and so it open to  */
	/* many really nasty attacks!                             */
        return vsprintf(s,format,ap);
}
#endif

struct mbus_key{
	char	*algorithm;
	char	*key;
	int	 key_len;
};

struct mbus_msg {
	struct mbus_msg	*next;
	struct timeval	 time;	/* Time the message was sent, to trigger a retransmit */
	struct timeval	 ts;	/* Time the message was composed, the timestamp in the packet header */
	char		*dest;
	int		 reliable;
	int		 complete;	/* Indicates that we've finished adding cmds to this message */
	int		 seqnum;
	int		 retransmit_count;
	int		 message_size;
	int		 num_cmds;
	char		*cmd_list[MBUS_MAX_QLEN];
	char		*arg_list[MBUS_MAX_QLEN];
};

struct mbus {
	socket_udp	 *s;
	int		  num_addr;
	char		 *addr[MBUS_MAX_ADDR];	/* Addresses we respond to. 			*/
	int		  max_other_addr;
	int		  num_other_addr;
	char		**other_addr;		/* Addresses of other entities on the mbus. 	*/
	char		 *parse_buffer[MBUS_MAX_PD];
	int		  parse_depth;
	int		  seqnum;
	void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
	void (*err_handler)(int seqnum, int reason);
	struct mbus_msg	 *cmd_queue;		/* Queue of messages waiting to be sent */
	struct mbus_msg	 *waiting_ack;		/* The last reliable message sent, if we have not yet got the ACK */
	char		 *hashkey;
	int		  hashkeylen;
	char		 *encrkey;
	int		  encrkeylen;
#ifdef WIN32
	HKEY		  cfgKey;
#else
	fd_t		  cfgfd;  		/* The file descriptor for the $HOME/.mbus config file, on Unix */
#endif
	int		  cfg_locked;
	struct timeval	  last_heartbeat;	/* Last time we sent a heartbeat message */
};

#define SECS_PER_WEEK    604800
#define MBUS_ENCRKEY_LEN      7
#define MBUS_HASHKEY_LEN     12

static char *mbus_new_encrkey(void)
{
	char		*key;	/* The key we are going to return... */
#ifdef MBUS_ENCRYPT_BY_DEFAULT
	/* Create a new key, for use by the hashing routines. Returns */
	/* a key of the form (DES,MTIzMTU2MTg5MTEyMQ==)               */
	char		 random_string[MBUS_ENCRKEY_LEN];
	char		 encoded_string[(MBUS_ENCRKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_ENCRKEY_LEN; i++) {
		random_string[i] = ((int32)lbl_random() | 0x000ff000) >> 24;
	}
	/* Step 2: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_ENCRKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_ENCRKEY_LEN, encoded_string, (MBUS_ENCRKEY_LEN*4/3)+4);

	/* Step 3: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 18);
	sprintf(key, "(DES,%s)", encoded_string);
#else
	key = (char *) xmalloc(9);
	sprintf(key, "(NOENCR)");
#endif
	return key;
}

static char *mbus_new_hashkey(void)
{
	/* Create a new key, for use by the hashing routines. Returns  */
	/* a key of the form (HMAC-MD5,MTIzMTU2MTg5MTEyMQ==)           */
	char		 random_string[MBUS_HASHKEY_LEN];
	char		 encoded_string[(MBUS_HASHKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i;
	char		*key;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_HASHKEY_LEN; i++) {
		random_string[i] = ((int32)lbl_random() | 0x000ff000) >> 24;
	}
	/* Step 2: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_HASHKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_HASHKEY_LEN, encoded_string, (MBUS_HASHKEY_LEN*4/3)+4);

	/* Step 3: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 23);
	sprintf(key, "(HMAC-MD5,%s)", encoded_string);

	return key;
}

static void mbus_lock_config_file(struct mbus *m)
{
#ifdef WIN32
	/* Open the registry and create the mbus entries if they don't exist   */
	/* already. The default contents of the registry are random encryption */
	/* and authentication keys, and node local scope.                      */
	HKEY			key    = HKEY_CURRENT_USER;
	LPCTSTR			subKey = "Software\\Mbone Applications\\mbus";
	DWORD			disp;
	char			buffer[MBUS_BUF_SIZE];
	LONG			status;

	status = RegCreateKeyEx(key, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &(m->cfgKey), &disp);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to open registry: %s\n", buffer);
		abort();
	}
	if (disp == REG_CREATED_NEW_KEY) {
		char	*hashkey = mbus_new_hashkey();
		char	*encrkey = mbus_new_encrkey();
		char	*scope   = "HOSTLOCAL";

		status = RegSetValueEx(m->cfgKey, "HASHKEY", 0, REG_SZ, hashkey, strlen(hashkey) + 1);
		if (status != ERROR_SUCCESS) {
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
			debug_msg("Unable to set hashkey: %s\n", buffer);
			abort();
		}	
		status = RegSetValueEx(m->cfgKey, "ENCRYPTIONKEY", 0, REG_SZ, encrkey, strlen(encrkey) + 1);
		if (status != ERROR_SUCCESS) {
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
			debug_msg("Unable to set encrkey: %s\n", buffer);
			abort();
		}	
		status = RegSetValueEx(m->cfgKey, "SCOPE", 0, REG_SZ, scope, strlen(scope) + 1);
		if (status != ERROR_SUCCESS) {
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
			debug_msg("Unable to set scope: %s\n", buffer);
			abort();
		}	
		debug_msg("Created new registry entry...\n");
	} else {
		debug_msg("Opened existing registry entry...\n");
	}
#else
	/* Obtain a valid lock on the mbus configuration file. This function */
	/* creates the file, if one does not exist. The default contents of  */
	/* this file are random authentication and encryption keys, and node */
	/* local scope.                                                      */
	struct flock	 l;
	struct stat	 s;
	struct passwd	*p;	
	char		*buf;
	char		*cfg_file;

	/* The getpwuid() stuff is to determine the users home directory, into which we */
	/* write a .mbus config file. The struct returned by getpwuid() is statically   */
	/* allocated, so it's not necessary to free it afterwards.                      */
	p = getpwuid(getuid());
	if (p == NULL) {
		perror("Unable to get passwd entry");
		abort();
	}
	cfg_file = (char *) xmalloc(strlen(p->pw_dir) + 6);
	sprintf(cfg_file, "%s/.mbus", p->pw_dir);
	m->cfgfd = open(cfg_file, O_RDWR | O_CREAT, 0600);
	if (m->cfgfd == -1) {
		perror("Unable to open mbus configuration file");
		abort();
	}
	xfree(cfg_file);

	/* We attempt to get a lock on the config file, blocking until  */
	/* the lock can be obtained. The only time this should block is */
	/* when another instance of this code has a write lock on the   */
	/* file, because the contents are being updated.                */
	l.l_type   = F_WRLCK;
	l.l_start  = 0;
	l.l_whence = SEEK_SET;
	l.l_len    = 0;
	if (fcntl(m->cfgfd, F_SETLKW, &l) == -1) {
		perror("Unable to lock mbus configuration file");
		abort();
	}

	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	if (s.st_size == 0) {
		/* Empty file, create with sensible defaults... */
		char	*hashkey = mbus_new_hashkey();
		char	*encrkey = mbus_new_encrkey();
		char	*scope   = "HOSTLOCAL";
		int	 len;

		len = strlen(hashkey) + strlen(encrkey) + strlen(scope) + 39;
		buf = (char *) xmalloc(len);
		sprintf(buf, "[MBUS]\nHASHKEY=%s\nENCRYPTIONKEY=%s\nSCOPE=%s\n", hashkey, encrkey, scope);
		write(m->cfgfd, buf, strlen(buf));
		xfree(buf);
		free(hashkey);
		xfree(encrkey);
		debug_msg("Wrote config file\n");
	} else {
		/* Read in the contents of the config file... */
		buf = (char *) xmalloc(s.st_size+1);
		memset(buf, '\0', s.st_size+1);
		if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
			perror("Unable to read config file\n");
			abort();
		}
		/* Check that the file contains sensible information...   */
		/* This is rather a pathetic check, but it'll do for now! */
		if (strncmp(buf, "[MBUS]", 6) != 0) {
			debug_msg("Mbus config file has incorrect header\n");
			abort();
		}
		xfree(buf);
	}
#endif
	m->cfg_locked = TRUE;
}

static void mbus_unlock_config_file(struct mbus *m)
{
#ifdef WIN32
	LONG status;
	char buffer[MBUS_BUF_SIZE];
	
	status = RegCloseKey(m->cfgKey);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to close registry: %s\n", buffer);
		abort();
	}
	debug_msg("Closed registry entry...\n");
#else
	struct flock	l;

	l.l_type   = F_UNLCK;
	l.l_start  = 0;
	l.l_whence = SEEK_SET;
	l.l_len    = 0;
	if (fcntl(m->cfgfd, F_SETLKW, &l) == -1) {
		perror("Unable to unlock mbus configuration file");
		abort();
	}
	close(m->cfgfd);
	m->cfgfd = -1;
#endif
	m->cfg_locked = FALSE;
}

#ifndef WIN32
static void mbus_get_key(struct mbus *m, struct mbus_key *key, char *id)
{
	struct stat	 s;
	char		*buf;
	char		*line;
	char		*tmp;
	int		 pos;

	assert(m->cfg_locked);

	if (lseek(m->cfgfd, 0, SEEK_SET) == -1) {
		perror("Can't seek to start of config file");
		abort();
	}
	if (fstat(m->cfgfd, &s) != 0) {
		perror("Unable to stat config file\n");
		abort();
	}
	/* Read in the contents of the config file... */
	buf = (char *) xmalloc(s.st_size+1);
	memset(buf, '\0', s.st_size+1);
	if (read(m->cfgfd, buf, s.st_size) != s.st_size) {
		perror("Unable to read config file\n");
		abort();
	}
	
	line = (char *) xmalloc(s.st_size+1);
	sscanf(buf, "%s", line);
	if (strcmp(line, "[MBUS]") != 0) {
		debug_msg("Invalid .mbus file\n");
		abort();
	}
	pos = strlen(line) + 1;
	while (pos < s.st_size) {
		sscanf(buf+pos, "%s", line);
		pos += strlen(line) + 1;
		if (strncmp(line, id, strlen(id)) == 0) {
			key->algorithm   = (char *) strdup(strtok(line+strlen(id), ",)"));
			if (strcmp(key->algorithm, "NOENCR") != 0) {
				key->key     = (char *) strtok(NULL  , ")");
				key->key_len = strlen(key->key);
				tmp = (char *) xmalloc(key->key_len);
				key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
				key->key = tmp;
			} else {
				key->key     = NULL;
				key->key_len = 0;
			}
			xfree(buf);
			xfree(line);
			return;
		}
	}
	debug_msg("Unable to read hashkey from config file\n");
	xfree(buf);
	xfree(line);
}
#endif

static void mbus_get_encrkey(struct mbus *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
	unsigned char	*des_key;
	int		 i, j, k;
#ifdef WIN32
	long		 status;
	DWORD		 type;
	char		*buffer;
	int	 	 buflen = MBUS_BUF_SIZE;
	char		*tmp;

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
	buffer = (char *) xmalloc(MBUS_BUF_SIZE);
	status = RegQueryValueEx(m->cfgKey, "ENCRYPTIONKEY", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get encrkey: %s\n", buffer);
		abort();
	}
	assert(type == REG_SZ);
	assert(buflen > 0);

	/* Parse the key... */
	key->algorithm   = strdup(strtok(buffer+1, ",)"));
	if (strcmp(key->algorithm, "NOENCR") != 0) {
		key->key     = (char *) strtok(NULL  , ")");
		key->key_len = strlen(key->key);
		tmp = (char *) xmalloc(key->key_len);
		key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
		key->key = tmp;
	} else {
		key->key     = NULL;
		key->key_len = 0;
	}

	debug_msg("alg=%s key=%s keylen=%d\n", key->algorithm, key->key, key->key_len);

	xfree(buffer);
#else
	mbus_get_key(m, key, "ENCRYPTIONKEY=(");
#endif
	if (strcmp(key->algorithm, "DES") == 0) {
		assert(key->key != NULL);
		assert(key->key_len == 7);
		/* Take the first 56-bits of the input key and spread it across   */
		/* the 64-bit DES key space inserting a bit-space of garbage      */
		/* (for parity) every 7 bits. The garbage will be taken care of   */
		/* below. The library we're using expects the key and parity bits */
		/* in the following MSB order: K0 K1...K6 P0 K8 K9...K14 P1...    */
		des_key = (unsigned char *) xmalloc(8);
		des_key[0] = key->key[0];
		des_key[1] = key->key[0] << 7 | key->key[1] >> 1;
		des_key[2] = key->key[1] << 6 | key->key[2] >> 2;
		des_key[3] = key->key[2] << 5 | key->key[3] >> 3;
		des_key[4] = key->key[3] << 4 | key->key[4] >> 4;
		des_key[5] = key->key[4] << 3 | key->key[5] >> 5;
		des_key[6] = key->key[5] << 2 | key->key[6] >> 6;
		des_key[7] = key->key[6] << 1;

		/* fill in parity bits to make DES library happy */
		for (i = 0; i < 8; ++i) 
		{
			k = des_key[i] & 0xfe;
			j = k;
			j ^= j >> 4;
			j ^= j >> 2;
			j ^= j >> 1;
			j = (j & 1) ^ 1;
			des_key[i] = k | j;
		}
		xfree(key->key);
		key->key     = des_key;
		key->key_len = 8;
	}
}

static void mbus_get_hashkey(struct mbus *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
#ifdef WIN32
	long	 status;
	DWORD	 type;
	char	*buffer;
	int	 buflen = MBUS_BUF_SIZE;
	char	*tmp;

	assert(m->cfg_locked);
	
	/* Read the key from the registry... */
	buffer = (char *) xmalloc(MBUS_BUF_SIZE);
	status = RegQueryValueEx(m->cfgKey, "HASHKEY", 0, &type, buffer, &buflen);
	if (status != ERROR_SUCCESS) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, status, 0, buffer, MBUS_BUF_SIZE, NULL);
		debug_msg("Unable to get encrkey: %s\n", buffer);
		abort();
	}
	assert(type == REG_SZ);
	assert(buflen > 0);

	/* Parse the key... */
	key->algorithm   = strdup(strtok(buffer+1, ","));
	key->key         = strtok(NULL  , ")");
	key->key_len     = strlen(key->key);

	debug_msg("alg=%s key=%s keylen=%d\n", key->algorithm, key->key, key->key_len);

	/* Decode the key... */
	tmp = (char *) xmalloc(key->key_len);
	key->key_len = base64decode(key->key, key->key_len, tmp, key->key_len);
	key->key = tmp;

	xfree(buffer);
#else
	mbus_get_key(m, key, "HASHKEY=(");
#endif
}

static int mbus_addr_match(char *a, char *b)
{
	/* Compare the addresses "a" and "b". These may optionally be */
	/* surrounded by "(" and ")" and may have an arbitrary amount */
	/* of white space between components of the addresses. There  */
	/* is a match if every word of address b is in address a.     */
	/* NOTE: The strings passed to this function are stored for   */
	/* later use and MUST NOT be modified by this routine.        */
	char	*y, c;

	assert(a != NULL);
	assert(b != NULL);

	/* Skip leading whitespace and '('... */
	while (isspace((unsigned char)*a) || (*a == '(')) a++;
	while (isspace((unsigned char)*b) || (*b == '(')) b++;

	while ((*b != '\0') && (*b != ')')) {
		while (isspace((unsigned char)*b)) b++;
		for (y = b; ((*y != ' ') && (*y != ')') && (*y != '\0')); y++) {
			/* do nothing */
		}
		c = *y;
		*y = '\0';
		if (strstr(a, b) == NULL) {
			/* ...this word not found */
			*y = c;
			return FALSE;
		}
		*y = c;
		b = y;
	}		
	return TRUE;
}

static void store_other_addr(struct mbus *m, char *a)
{
	/* This takes the address a and ensures it is stored in the   */
	/* m->other_addr field of the mbus structure. The other_addr  */
	/* field should probably be a hash table, but for now we hope */
	/* that there are not too many entities on the mbus, so the   */
	/* list is small.                                             */
	int	i;

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], a)) {
			/* Already in the list... */
			return;
		}
	}

	if (m->num_other_addr == m->max_other_addr) {
		/* Expand the list... */
		m->max_other_addr *= 2;
		m->other_addr = (char **) xrealloc(m->other_addr, m->max_other_addr * sizeof(char *));
	}
	m->other_addr[m->num_other_addr++] = xstrdup(a);
}

int mbus_addr_valid(struct mbus *m, char *addr)
{
	int	i;

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], addr)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* The tx_* functions are used to build an mbus message up in the */
/* tx_buffer, and to add authentication and encryption before the */
/* message is sent.                                               */
static char	 tx_cryptbuf[MBUS_BUF_SIZE];
static char	 tx_buffer[MBUS_BUF_SIZE];
static char	*tx_bufpos;

#define MBUS_AUTH_LEN 25

static void tx_header(int seqnum, int ts, char reliable, char *src, char *dst, int ackseq)
{
	memset(tx_buffer,   0, MBUS_BUF_SIZE);
	memset(tx_buffer, ' ', MBUS_AUTH_LEN);
	tx_bufpos = tx_buffer + MBUS_AUTH_LEN;
	sprintf(tx_bufpos, "\nmbus/1.0 %6d %9d %c (%s) %s ", seqnum, ts, reliable, src, dst);
	tx_bufpos += 33 + strlen(src) + strlen(dst);
	if (ackseq == -1) {
		sprintf(tx_bufpos, "()\n");
		tx_bufpos += 3;
	} else {
		sprintf(tx_bufpos, "(%6d)\n", ackseq);
		tx_bufpos += 9;
	}
}

static void tx_add_command(char *cmnd, char *args)
{
	sprintf(tx_bufpos, "%s (%s)\n", cmnd, args);
	tx_bufpos += strlen(cmnd) + strlen(args) + 4;
}

static void tx_send(struct mbus *m)
{
	char		digest[16];
	int		len;
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	while (((tx_bufpos - tx_buffer) % 8) != 0) {
		/* Pad to a multiple of 8 bytes, so the encryption can work... */
		*(tx_bufpos++) = ' ';
	}
	*tx_bufpos = '\0';
	len = tx_bufpos - tx_buffer;

	if (m->hashkey != NULL) {
		/* Authenticate... */
		hmac_md5(tx_buffer + MBUS_AUTH_LEN, strlen(tx_buffer) - MBUS_AUTH_LEN, m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 16, tx_buffer, MBUS_AUTH_LEN - 1);
	}
	if (m->encrkey != NULL) {
		/* Encrypt... */
		memset(tx_cryptbuf, 0, MBUS_BUF_SIZE);
		memcpy(tx_cryptbuf, tx_buffer, len);
		assert((len % 8) == 0);
		assert(len < MBUS_BUF_SIZE);
		assert(m->encrkeylen == 8);
		qfDES_CBC_e(m->encrkey, tx_cryptbuf, len, initVec);
		memcpy(tx_buffer, tx_cryptbuf, len);
	}
	udp_send(m->s, tx_buffer, len);
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	int	 i;

	tx_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
	for (i = 0; i < curr->num_cmds; i++) {
		tx_add_command(curr->cmd_list[i], curr->arg_list[i]);
	}
	tx_send(m);
	curr->retransmit_count++;
}

void mbus_retransmit(struct mbus *m)
{
	struct mbus_msg	*curr = m->waiting_ack;
	struct timeval	time;
	long		diff;

	if (!mbus_waiting_ack(m)) {
		return;
	}

	gettimeofday(&time, NULL);

	/* diff is time in milliseconds that the message has been awaiting an ACK */
	diff = ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - ((curr->time.tv_sec * 1000) + (curr->time.tv_usec / 1000));
	if (diff > 10000) {
		debug_msg("Reliable mbus message failed!\n");
		if (m->err_handler == NULL) {
			abort();
		}
		m->err_handler(curr->seqnum, MBUS_MESSAGE_LOST);
		return;
	} 
	/* Note: We only send one retransmission each time, to avoid
	 * overflowing the receiver with a burst of requests...
	 */
	if ((diff > 750) && (curr->retransmit_count == 2)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 500) && (curr->retransmit_count == 1)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 250) && (curr->retransmit_count == 0)) {
		resend(m, curr);
		return;
	}
	curr = curr->next;
}

void mbus_heartbeat(struct mbus *m, int interval)
{
	struct timeval	curr_time;

	gettimeofday(&curr_time, NULL);

	if (curr_time.tv_sec - m->last_heartbeat.tv_sec > interval) {
		mbus_qmsg(m, "()", "mbus.hello", "", FALSE);
		m->last_heartbeat = curr_time;
	}
}

int mbus_waiting_ack(struct mbus *m)
{
	return m->waiting_ack != NULL;
}

struct mbus *mbus_init(void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum, int reason))
{
	struct mbus	*m;
	struct mbus_key	 k;
	int		 i;

	m = (struct mbus *) xmalloc(sizeof(struct mbus));
	if (m == NULL) {
		debug_msg("Unable to allocate memory for mbus\n");
		return NULL;
	}

	mbus_lock_config_file(m);
	m->s		  = udp_init("224.255.222.239", (u_int16) 47000, 0);
	m->seqnum         = 0;
	m->cmd_handler    = cmd_handler;
	m->err_handler	  = err_handler;
	m->num_addr       = 0;
	m->num_other_addr = 0;
	m->max_other_addr = 10;
	m->other_addr     = (char **) xmalloc(sizeof(char *) * 10);
	m->parse_depth    = 0;
	m->cmd_queue	  = NULL;
	m->waiting_ack	  = NULL;

	gettimeofday(&(m->last_heartbeat), NULL);

	mbus_get_encrkey(m, &k);
	m->encrkey    = k.key;
	m->encrkeylen = k.key_len;

	mbus_get_hashkey(m, &k);
	m->hashkey    = k.key;
	m->hashkeylen = k.key_len;

	for (i = 0; i < MBUS_MAX_ADDR; i++) m->addr[i]         = NULL;
	for (i = 0; i < MBUS_MAX_PD;   i++) m->parse_buffer[i] = NULL;
	mbus_unlock_config_file(m);

	return m;
}

static void mbus_flush_msgs(struct mbus_msg *queue)
{
        struct mbus_msg *curr, *next;
        int i;

        curr = queue;
        while(curr) {
                next = curr->next;
                xfree(curr->dest);
                for(i = 0; i < curr->num_cmds; i++) {
                        xfree(curr->cmd_list[i]);
                        xfree(curr->arg_list[i]);
                }
                curr = next;
        }
}

void mbus_exit(struct mbus *m) 
{
        assert(m != NULL);

        while(m->parse_depth--) {
                xfree(m->parse_buffer[m->parse_depth]);
        }

        mbus_flush_msgs(m->cmd_queue);
        mbus_flush_msgs(m->waiting_ack);

        if (m->encrkey != NULL) {
                xfree(m->encrkey);
        }
        xfree(m->hashkey);
        xfree(m);
}

void mbus_addr(struct mbus *m, char *addr)
{
	assert(m->num_addr < MBUS_MAX_ADDR);
	mbus_parse_init(m, xstrdup(addr));
	if (mbus_parse_lst(m, &(m->addr[m->num_addr]))) {
		m->num_addr++;
	}
	mbus_parse_done(m);
}

void mbus_send(struct mbus *m)
{
	/* Send one, or more, messages previosly queued with mbus_qmsg(). */
	/* Messages for the same destination are batched together. Stops  */
	/* when a reliable message is sent, until the ACK is received.    */
	struct mbus_msg	*curr = m->cmd_queue;
	int		 i;

	if (m->waiting_ack != NULL) {
		return;
	}

	while (curr != NULL) {
		/* Create the message... */
		tx_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
		for (i = 0; i < curr->num_cmds; i++) {
			tx_add_command(curr->cmd_list[i], curr->arg_list[i]);
		}
		tx_send(m);
		
		m->cmd_queue = curr->next;
		if (curr->reliable) {
			/* Reliable message, wait for the ack... */
			gettimeofday(&(curr->time), NULL);
			m->waiting_ack = curr;
			return;
		} else {
			while (curr->num_cmds > 0) {
				curr->num_cmds--;
				xfree(curr->cmd_list[curr->num_cmds]);
				xfree(curr->arg_list[curr->num_cmds]);
			}
			xfree(curr->dest);
			xfree(curr);
		}
		curr = m->cmd_queue;
	}
}

void mbus_qmsg(struct mbus *m, char *dest, const char *cmnd, const char *args, int reliable)
{
	/* Queue up a message for sending. The message is not */
	/* actually sent until mbus_send() is called.         */
	struct mbus_msg	*curr = m->cmd_queue;
	struct mbus_msg	*prev = NULL;
	int		 alen = strlen(cmnd) + strlen(args) + 4;

	if (reliable && !mbus_addr_valid(m, dest)) {
		debug_msg("Trying to send reliably to an unknown address...\n");
#ifdef NDEF
		if (m->err_handler == NULL) {
			abort();
		}
		m->err_handler(curr->seqnum, MBUS_DESTINATION_UNKNOWN);
#endif
	}

	while (curr != NULL) {
		if ((!curr->complete)
		&& mbus_addr_match(curr->dest, dest) 
		&& (curr->num_cmds < MBUS_MAX_QLEN) 
		&& ((curr->message_size + alen) < (MBUS_BUF_SIZE - 8))) {
			/* Slots message in if it fits, but this breaks ordering.  Msg
		         * X+1 maybe shorter than X that is in next packet, so X+1 jumps
		         * ahead.
		         */
			curr->num_cmds++;
			curr->reliable |= reliable;
			curr->cmd_list[curr->num_cmds-1] = xstrdup(cmnd);
			curr->arg_list[curr->num_cmds-1] = xstrdup(args);
			curr->message_size += alen;
			return;
		} else {
			curr->complete = TRUE;
		}
		prev = curr;
		curr = curr->next;
	}
	curr = (struct mbus_msg *) xmalloc(sizeof(struct mbus_msg));
	curr->next             = NULL;
	curr->dest             = xstrdup(dest);
	curr->retransmit_count = 0;
	curr->message_size     = alen + 60 + strlen(dest) + strlen(m->addr[0]);
	curr->seqnum           = m->seqnum++;
	curr->reliable         = reliable;
	curr->complete         = FALSE;
	curr->num_cmds         = 1;
	curr->cmd_list[0]      = xstrdup(cmnd);
	curr->arg_list[0]      = xstrdup(args);
	if (prev == NULL) {
		m->cmd_queue = curr;
	} else {
		prev->next = curr;
	}
	gettimeofday(&(curr->time), NULL);
	gettimeofday(&(curr->ts),   NULL);
}

void mbus_qmsgf(struct mbus *m, char *dest, int reliable, const char *cmnd, const char *format, ...)
{
	/* This is a wrapper around mbus_qmsg() which does a printf() style format into  */
	/* a buffer. Saves the caller from having to a a malloc(), write the args string */
	/* and then do a free(), and also saves worring about overflowing the buffer, so */
	/* removing a common source of bugs!                                             */
	char	buffer[MBUS_BUF_SIZE];
	va_list	ap;

	va_start(ap, format);
#ifdef WIN32
        _vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#else
        vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#endif
	va_end(ap);
	mbus_qmsg(m, dest, cmnd, buffer, reliable);
}

void mbus_parse_init(struct mbus *m, char *str)
{
	assert(m->parse_depth < (MBUS_MAX_PD - 1));
	m->parse_buffer[++m->parse_depth] = str;
}

void mbus_parse_done(struct mbus *m)
{
	m->parse_depth--;
	assert(m->parse_depth >= 0);
}

int mbus_parse_lst(struct mbus *m, char **l)
{
	int instr = FALSE;
	int inlst = FALSE;

	*l = m->parse_buffer[m->parse_depth];
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }
	if (*m->parse_buffer[m->parse_depth] != '(') {
		return FALSE;
	}
	*(m->parse_buffer[m->parse_depth]) = ' ';
	while (*m->parse_buffer[m->parse_depth] != '\0') {
		if ((*m->parse_buffer[m->parse_depth] == '"') && (*(m->parse_buffer[m->parse_depth]-1) != '\\')) {
			instr = !instr;
		}
		if ((*m->parse_buffer[m->parse_depth] == '(') && (*(m->parse_buffer[m->parse_depth]-1) != '\\') && !instr) {
			inlst = !inlst;
		}
		if ((*m->parse_buffer[m->parse_depth] == ')') && (*(m->parse_buffer[m->parse_depth]-1) != '\\') && !instr) {
			if (inlst) {
				inlst = !inlst;
			} else {
				*m->parse_buffer[m->parse_depth] = '\0';
				m->parse_buffer[m->parse_depth]++;
				return TRUE;
			}
		}
		m->parse_buffer[m->parse_depth]++;
	}
	return FALSE;
}

int mbus_parse_str(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }
	if (*m->parse_buffer[m->parse_depth] != '"') {
		return FALSE;
	}
	*s = m->parse_buffer[m->parse_depth]++;
	while (*m->parse_buffer[m->parse_depth] != '\0') {
		if ((*m->parse_buffer[m->parse_depth] == '"') && (*(m->parse_buffer[m->parse_depth]-1) != '\\')) {
			m->parse_buffer[m->parse_depth]++;
			*m->parse_buffer[m->parse_depth] = '\0';
			m->parse_buffer[m->parse_depth]++;
			return TRUE;
		}
		m->parse_buffer[m->parse_depth]++;
	}
	return FALSE;
}

static int mbus_parse_sym(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }
	if (!isgraph((unsigned char)*m->parse_buffer[m->parse_depth])) {
		return FALSE;
	}
	*s = m->parse_buffer[m->parse_depth]++;
	while (!isspace((unsigned char)*m->parse_buffer[m->parse_depth]) && (*m->parse_buffer[m->parse_depth] != '\0')) {
		m->parse_buffer[m->parse_depth]++;
	}
	*m->parse_buffer[m->parse_depth] = '\0';
	m->parse_buffer[m->parse_depth]++;
	return TRUE;
}

int mbus_parse_int(struct mbus *m, int *i)
{
	char	*p;

        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }

	*i = strtol(m->parse_buffer[m->parse_depth], &p, 10);
	if (((*i == LONG_MAX) || (*i == LONG_MIN)) && (errno == ERANGE)) {
		debug_msg("integer out of range\n");
		return FALSE;
	}

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
	return TRUE;
}

int mbus_parse_flt(struct mbus *m, double *d)
{
	char	*p;
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }

	*d = strtod(m->parse_buffer[m->parse_depth], &p);
	if (errno == ERANGE) {
		debug_msg("float out of range\n");
		return FALSE;
	}

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
	return TRUE;
}

char *mbus_decode_str(char *s)
{
	int	l = strlen(s);
	int	i, j;

	/* Check that this an encoded string... */
	assert(s[0]   == '\"');
	assert(s[l-1] == '\"');

	for (i=1,j=0; i < l - 1; i++,j++) {
		if (s[i] == '\\') {
			i++;
		}
		s[j] = s[i];
	}
	s[j] = '\0';
	return s;
}

char *mbus_encode_str(const char *s)
{
	int 	 i, j;
	int	 len = strlen(s);
	char	*buf = (char *) xmalloc((len * 2) + 3);

	for (i = 0, j = 1; i < len; i++,j++) {
		if (s[i] == ' ') {
			buf[j] = '\\';
			buf[j+1] = ' ';
			j++;
		} else if (s[i] == '\"') {
			buf[j] = '\\';
			buf[j+1] = '\"';
			j++;
		} else {
			buf[j] = s[i];
		}
	}
	buf[0]   = '\"';
	buf[j]   = '\"';
	buf[j+1] = '\0';
	return buf;
}

int mbus_recv(struct mbus *m, void *data, struct timeval *timeout)
{
	char		*auth, *ver, *src, *dst, *ack, *r, *cmd, *param;
	char	 	buffer[MBUS_BUF_SIZE];
	int	 	buffer_len, seq, i, a, rx, ts, authlen;
	char	 	ackbuf[MBUS_ACK_BUF_SIZE];
	char	 	digest[16];
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	rx = FALSE;
	while (1) {
		memset(buffer, 0, MBUS_BUF_SIZE);
                assert(m->s != NULL);
		udp_fd_zero();
		udp_fd_set(m->s);
                if ((udp_select(timeout) > 0) && udp_fd_isset(m->s)) {
			buffer_len = udp_recv(m->s, buffer, MBUS_BUF_SIZE);
			if (buffer_len > 0) {
				rx = TRUE;
			} else {
				return rx;
			}
		} else {
			return FALSE;
		}

		if (m->encrkey != NULL) {
			/* Decrypt the message... */
			if ((buffer_len % 8) != 0) {
				debug_msg("Encrypted message not a multiple of 8 bytes in length\n");
				continue;
			}
			memcpy(tx_cryptbuf, buffer, buffer_len);
			memset(initVec, 0, 8);
			qfDES_CBC_d(m->encrkey, tx_cryptbuf, buffer_len, initVec);
			if (strncmp(tx_cryptbuf + MBUS_AUTH_LEN + 1, "mbus/1.0", 8) != 0) {
				debug_msg("Message did not correctly decrypt\n");
				continue;
			}
			memcpy(buffer, tx_cryptbuf, buffer_len);
		}

		mbus_parse_init(m, buffer);
		/* Parse the authentication header */
		if (!mbus_parse_sym(m, &auth)) {
			debug_msg("Failed to parse authentication header\n");
			mbus_parse_done(m);
			continue;
		}

		/* Check that the packet authenticates correctly... */
		authlen = strlen(auth);
		hmac_md5(buffer + authlen + 1, buffer_len - authlen - 1, m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 16, ackbuf, 24);
		if ((strlen(auth) != 24) || (strncmp(auth, ackbuf, 24) != 0)) {
			debug_msg("Failed to authenticate message...\n");
			mbus_parse_done(m);
			continue;
		}

		/* Parse the header */
		if (!mbus_parse_sym(m, &ver)) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (1): %s\n",ver);
			continue;
		}
		if (strcmp(ver, "mbus/1.0") != 0) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (2): %s\n",ver);
			continue;
		}
		if (!mbus_parse_int(m, &seq)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq\n");
			continue;
		}
		if (!mbus_parse_int(m, &ts)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ts\n");
			continue;
		}
		if (!mbus_parse_sym(m, &r)) {
			mbus_parse_done(m);
			debug_msg("Parser failed reliable\n");
			continue;
		}
		if (!mbus_parse_lst(m, &src)) {
			mbus_parse_done(m);
			debug_msg("Parser failed src\n");
			continue;
		}
		if (!mbus_parse_lst(m, &dst)) {
			mbus_parse_done(m);
			debug_msg("Parser failed dst\n");
			continue;
		}
		if (!mbus_parse_lst(m, &ack)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ack\n");
			continue;
		}

		store_other_addr(m, src);

		/* Check if the message was addressed to us... */
		for (i = 0; i < m->num_addr; i++) {
			if (mbus_addr_match(m->addr[i], dst)) {
				/* ...if so, process any ACKs received... */
				mbus_parse_init(m, ack);
				while (mbus_parse_int(m, &a)) {
					if (mbus_waiting_ack(m) && (m->waiting_ack->seqnum == a)) {
						while (m->waiting_ack->num_cmds > 0) {
							m->waiting_ack->num_cmds--;
							xfree(m->waiting_ack->cmd_list[m->waiting_ack->num_cmds]);
							xfree(m->waiting_ack->arg_list[m->waiting_ack->num_cmds]);
						}
						xfree(m->waiting_ack->dest);
						xfree(m->waiting_ack);
						m->waiting_ack = NULL;
					}
				}
				mbus_parse_done(m);
				/* ...if an ACK was requested, send one... */
				if (strcmp(r, "R") == 0) {
					char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
					struct timeval	 t;

					sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
					gettimeofday(&t, NULL);
					tx_header(++m->seqnum, (int) t.tv_sec, 'U', m->addr[0], newsrc, seq);
					tx_send(m);
					xfree(newsrc);
				}
				/* ...and process the commands contained in the message */
				while (mbus_parse_sym(m, &cmd)) {
					if (mbus_parse_lst(m, &param)) {
						m->cmd_handler(src, cmd, param, data);
					} else {
						debug_msg("Unable to parse mbus command:\n");
						debug_msg("cmd = %s\n", cmd);
						debug_msg("arg = %s\n", param);
						break;
					}
				}
			}
		}
		mbus_parse_done(m);
	}
}

