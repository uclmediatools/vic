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

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "net_udp.h"
#include "hmac.h"
#include "base64.h"
#include "crypt_random.h"
#include "mbus.h"

#define MBUS_BUF_SIZE	 1500
#define MBUS_ACK_BUF_SIZE 110
#define MBUS_MAX_ADDR	   10
#define MBUS_MAX_PD	   10
#define MBUS_MAX_QLEN	   50 /* Number of messages we can queue with mbus_qmsg() */

struct mbus_key{
	char	*algorithm;
	char	*key;
	int	 key_len;
	u_int32	 expiry_time;
};

struct mbus_msg {
	struct mbus_msg	*next;
	struct timeval	time;
	char		*dest;
	int		 reliable;
	int		 seqnum;
	int		 retransmit_count;
	int		 message_size;
	int		 num_cmds;
	char		*cmd_list[MBUS_MAX_QLEN];
	char		*arg_list[MBUS_MAX_QLEN];
};

struct mbus {
	socket_udp	*s;
	unsigned short	 channel;
	int		 num_addr;
	char		*addr[MBUS_MAX_ADDR];
	char		*parse_buffer[MBUS_MAX_PD];
	int		 parse_depth;
	int		 seqnum;
	void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
	void (*err_handler)(int seqnum);
	struct mbus_msg	*cmd_queue;
	struct mbus_msg	*waiting_ack;
	char		*hashkey;
	int		 hashkeylen;
	char		*encrkey;
	int		 encrkeylen;
#ifndef WIN32
	fd_t		 cfgfd;	  /* The file descriptor for the $HOME/.mbus config file, on Unix */
#endif
	int		 cfg_locked;
};

#define SECS_PER_WEEK    604800
#define MBUS_ENCRKEY_LEN      7
#define MBUS_HASHKEY_LEN     12

static char *mbus_new_encrkey(void)
{
	/* Create a new key, for use by the hashing routines. Returns */
	/* a key of the form (DES,946080000,MTIzMTU2MTg5MTEyMQ==)     */
	struct timeval	 curr_time;
	u_int32		 expiry_time;
	char		 random_string[MBUS_ENCRKEY_LEN];
	char		 encoded_string[(MBUS_ENCRKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i;
	char		*key;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_ENCRKEY_LEN; i++) {
		random_string[i] = (lbl_random() | 0x000ff000) >> 24;
	}
	/* Step 2: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_ENCRKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_ENCRKEY_LEN, encoded_string, (MBUS_ENCRKEY_LEN*4/3)+4);

	/* Step 3: figure out the expiry time of the key, */
	/*         we use a value one week from now.      */
	gettimeofday(&curr_time, NULL);
	expiry_time = curr_time.tv_sec + SECS_PER_WEEK;

	/* Step 4: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 18);
	sprintf(key, "(DES,%ld,%s)", expiry_time, encoded_string);

	return key;
}

static char *mbus_new_hashkey(void)
{
	/* Create a new key, for use by the hashing routines. Returns  */
	/* a key of the form (HMAC-MD5,946080000,MTIzMTU2MTg5MTEyMQ==) */
	struct timeval	 curr_time;
	u_int32		 expiry_time;
	char		 random_string[MBUS_HASHKEY_LEN];
	char		 encoded_string[(MBUS_HASHKEY_LEN*4/3)+4];
	int		 encoded_length;
	int		 i;
	char		*key;

	/* Step 1: generate a random string for the key... */
	for (i = 0; i < MBUS_HASHKEY_LEN; i++) {
		random_string[i] = (lbl_random() | 0x000ff000) >> 24;
	}
	/* Step 2: base64 encode that string... */
	memset(encoded_string, 0, (MBUS_HASHKEY_LEN*4/3)+4);
	encoded_length = base64encode(random_string, MBUS_HASHKEY_LEN, encoded_string, (MBUS_HASHKEY_LEN*4/3)+4);

	/* Step 3: figure out the expiry time of the key, */
	/*         we use a value one week from now.      */
	gettimeofday(&curr_time, NULL);
	expiry_time = curr_time.tv_sec + SECS_PER_WEEK;

	/* Step 4: put it all together to produce the key... */
	key = (char *) xmalloc(encoded_length + 23);
	sprintf(key, "(HMAC-MD5,%ld,%s)", expiry_time, encoded_string);

	return key;
}

static void mbus_lock_config_file(struct mbus *m)
{
#ifdef WIN32
	/* Do something complicated with the registry... */
#else
	/* Obtain a valid lock on the mbus configuration file. This function */
	/* creates the file, if one does not exist. The default contents of  */
	/* this file are a random authentication key, no encryption and node */
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
	/* Do something complicated with the registry... */
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

static void mbus_get_key(struct mbus *m, struct mbus_key *key, char *id)
{
	struct stat	 s;
	char		*buf;
	char		*line;
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
		debug_msg("%s %d %d\n", line, pos, s.st_size);
		if (strncmp(line, id, 9) == 0) {
#ifdef NDEF
			key->algorithm   = strdup(strtok(line+9, ","));
			key->expiry_time =   atol(strtok(NULL  , ","));
			key->key         = strdup(strtok(NULL  , ")"));
#else			
			key->algorithm   = NULL;
			key->expiry_time = 0;
			key->key         = NULL;
			key->key_len     = 0;
#endif
			xfree(buf);
			xfree(line);
			return;
		}
	}
	debug_msg("Unable to read hashkey from config file\n");
	xfree(buf);
	xfree(line);
}

static void mbus_get_encrkey(struct mbus *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
#ifdef WIN32
	/* Do something complicated with the registry... */
#else
	mbus_get_key(m, key, "ENCRYPTIONKEY=(");
#endif
}

static void mbus_get_hashkey(struct mbus *m, struct mbus_key *key)
{
	/* This MUST be called while the config file is locked! */
#ifdef WIN32
	/* Do something complicated with the registry... */
#else
	mbus_get_key(m, key, "HASHKEY=(");
#endif
}

static int mbus_addr_match(char *a, char *b)
{
	assert(a != NULL);
	assert(b != NULL);

	while ((*a != '\0') && (*b != '\0')) {
		while (isspace((unsigned char)*a)) a++;
		while (isspace((unsigned char)*b)) b++;
		if (*a == '*') {
			a++;
			if ((*a != '\0') && !isspace((unsigned char)*a)) {
				return FALSE;
			}
			while(!isspace((unsigned char)*b) && (*b != '\0')) b++;
		}
		if (*b == '*') {
			b++;
			if ((*b != '\0') && !isspace((unsigned char)*b)) {
				return FALSE;
			}
			while(!isspace((unsigned char)*a) && (*a != '\0')) a++;
		}
		if (*a != *b) {
			return FALSE;
		}
		a++;
		b++;
	}
	return TRUE;
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	char	 digest[16];
	char	 buffer[MBUS_BUF_SIZE];
	char	*bufp = buffer;
	int	 i;

	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	memset(buffer, 0, MBUS_BUF_SIZE);
	sprintf(bufp, "########################\nmbus/1.0 %6d %c (%s) %s ()\n", curr->seqnum, curr->reliable?'R':'U', m->addr[0], curr->dest);
	hmac_md5(buffer+25, strlen(buffer)-25, m->hashkey, m->hashkeylen, digest);
	base64encode(digest, 16, buffer, 24);
	bufp += strlen(m->addr[0]) + strlen(curr->dest) + 50;
	for (i = 0; i < curr->num_cmds; i++) {
		sprintf(bufp, "%s (%s)\n", curr->cmd_list[i], curr->arg_list[i]);
		bufp += strlen(curr->cmd_list[i]) + strlen(curr->arg_list[i]) + 4;
	}

	debug_msg("resending %d\n", curr->seqnum);
	udp_send(m->s, buffer, bufp - buffer);
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
		m->err_handler(curr->seqnum);
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

int mbus_waiting_ack(struct mbus *m)
{
	return m->waiting_ack != NULL;
}

struct mbus *mbus_init(unsigned short channel, 
                       void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum))
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
	m->s		= udp_init("224.255.222.239", (u_int16) (47000 + channel), 0);
	m->channel	= channel;
	m->seqnum       = 0;
	m->cmd_handler  = cmd_handler;
	m->err_handler	= err_handler;
	m->num_addr     = 0;
	m->parse_depth  = 0;
	m->cmd_queue	= NULL;
	m->waiting_ack	= NULL;

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
	char		 buffer[MBUS_BUF_SIZE];
	char		*bufp;
	struct mbus_msg	*curr = m->cmd_queue;
	int		 i;
	char		 digest[16];

	if (m->waiting_ack != NULL) {
		return;
	}

	while (curr != NULL) {
		/* Create the message... */
		bufp = buffer;
		memset(buffer, 0, MBUS_BUF_SIZE);
		sprintf(bufp, "########################\nmbus/1.0 %6d %c (%s) %s ()\n", curr->seqnum, curr->reliable?'R':'U', m->addr[0], curr->dest);
		bufp += strlen(m->addr[0]) + strlen(curr->dest) + 50;
		for (i = 0; i < curr->num_cmds; i++) {
			int cmdlen = strlen(curr->cmd_list[i]) + strlen(curr->arg_list[i]) + 4;
			assert((bufp + cmdlen - buffer) < MBUS_BUF_SIZE);
			sprintf(bufp, "%s (%s)\n", curr->cmd_list[i], curr->arg_list[i]);
			bufp += cmdlen;
		}
		assert(*buffer && strlen(buffer) < MBUS_BUF_SIZE);

		/* Add an authentication header... this overwrites */
		/* the string of #'s at the start of the message.  */
		hmac_md5(buffer+25, strlen(buffer)-25, m->hashkey, m->hashkeylen, digest);
		i = base64encode(digest, 16, buffer, 24);
		
		/* Send the message... */
		udp_send(m->s, buffer, bufp - buffer);
		m->cmd_queue = curr->next;
		if (curr->reliable) {
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

	while (curr != NULL) {
		if (mbus_addr_match(curr->dest, dest) && (curr->num_cmds < MBUS_MAX_QLEN) && ((curr->message_size + alen) < MBUS_BUF_SIZE)) {
			curr->num_cmds++;
			curr->reliable |= reliable;
			curr->cmd_list[curr->num_cmds-1] = xstrdup(cmnd);
			curr->arg_list[curr->num_cmds-1] = xstrdup(args);
			curr->message_size += alen;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	curr = (struct mbus_msg *) xmalloc(sizeof(struct mbus_msg));
	curr->next             = NULL;
	curr->dest             = xstrdup(dest);
	curr->retransmit_count = 0;
	curr->message_size     = alen + 50 + strlen(dest) + strlen(m->addr[0]);
	curr->seqnum           = m->seqnum++;
	curr->reliable         = reliable;
	curr->num_cmds         = 1;
	curr->cmd_list[0]      = xstrdup(cmnd);
	curr->arg_list[0]      = xstrdup(args);
	if (prev == NULL) {
		m->cmd_queue = curr;
	} else {
		prev->next = curr;
	}
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
	*i = strtol(m->parse_buffer[m->parse_depth], &p, 10);

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
	*d = strtod(m->parse_buffer[m->parse_depth], &p);

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

int mbus_recv(struct mbus *m, void *data)
{
	char	*auth, *ver, *src, *dst, *ack, *r, *cmd, *param;
	char	 buffer[MBUS_BUF_SIZE];
	int	 buffer_len, seq, i, a, rx;
	char	 ackbuf[MBUS_ACK_BUF_SIZE];
	char	 digest[16];
	struct timeval	t;

	t.tv_sec  = 0;
	t.tv_usec = 0;

	rx = FALSE;
	while (1) {
		memset(buffer, 0, MBUS_BUF_SIZE);
                assert(m->s != NULL);
		udp_fd_zero();
		udp_fd_set(m->s);
		if ((udp_select(&t) > 0) && udp_fd_isset(m->s)) {
			buffer_len = udp_recv(m->s, buffer, MBUS_BUF_SIZE);
			if (buffer_len > 0) {
				rx = TRUE;
			} else {
				return rx;
			}
		} else {
			return FALSE;
		}

		mbus_parse_init(m, buffer);
		/* Parse the authentication header */
		if (!mbus_parse_sym(m, &auth)) {
			mbus_parse_done(m);
			debug_msg("Parser failed authentication header\n");
			return FALSE;
		}
		/* Check that the packet authenticates correctly... */
		hmac_md5(buffer + strlen(auth) + 1, buffer_len - strlen(auth) - 1, m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 16, ackbuf, 24);
		if ((strlen(auth) != 24) || (strncmp(auth, ackbuf, 24) != 0)) {
			mbus_parse_done(m);
			debug_msg("Unable to authenticate message\n");
			return FALSE;
		}

		/* Parse the header */
		if (!mbus_parse_sym(m, &ver)) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (1): %s\n",ver);
			return FALSE;
		}
		if (strcmp(ver, "mbus/1.0") != 0) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (2): %s\n",ver);
			return FALSE;
		}
		if (!mbus_parse_int(m, &seq)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq: %s\n", seq);
			return FALSE;
		}
		if (!mbus_parse_sym(m, &r)) {
			mbus_parse_done(m);
			debug_msg("Parser failed reliable: %s\n", seq);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &src)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq: %s\n", src);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &dst)) {
			mbus_parse_done(m);
			debug_msg("Parser failed dst: %s\n", dst);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &ack)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ack: %s\n", ack);
			return FALSE;
		}

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
					sprintf(ackbuf, "########################\nmbus/1.0 %d U (%s) (%s) (%d)\n", ++m->seqnum, m->addr[0], src, seq);
					assert(strlen(ackbuf)< MBUS_ACK_BUF_SIZE);
					hmac_md5(ackbuf+25, strlen(ackbuf)-25, m->hashkey, m->hashkeylen, digest);
					base64encode(digest, 16, ackbuf, 24);
					udp_send(m->s, ackbuf, strlen(ackbuf));
				}
				/* ...and process the commands contained in the message */
				while (mbus_parse_sym(m, &cmd)) {
					if (mbus_parse_lst(m, &param)) {
						m->cmd_handler(src, cmd, param, data);
					} else {
						debug_msg("Unable to parse mbus command paramaters...\n");
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

