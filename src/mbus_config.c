/*
 * FILE:    mbus_config.c
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1999 University College London
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
#include "crypt_random.h"
#include "base64.h"
#include "mbus.h"
#include "mbus_config.h"

#define MBUS_ENCRYPT_BY_DEFAULT
#define MBUS_ENCRKEY_LEN      7
#define MBUS_HASHKEY_LEN     12

char *mbus_new_encrkey(void)
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

char *mbus_new_hashkey(void)
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

void mbus_lock_config_file(struct mbus_config *m)
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

void mbus_unlock_config_file(struct mbus_config *m)
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
static void mbus_get_key(struct mbus_config *m, struct mbus_key *key, char *id)
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

void mbus_get_encrkey(struct mbus_config *m, struct mbus_key *key)
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

void mbus_get_hashkey(struct mbus_config *m, struct mbus_key *key)
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

