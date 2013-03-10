#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

#include "ivepkg.h"
#include "sqlite3.h"

static sqlite3 *db;

char * itoa(int val, int base)
{ //@{
	static char buf[32] = {0};
	int i = 30;

	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];

	return &buf[i+1];

} //@}	

char * sha1(char * filename, char * fakeroot)
{ //@{
	char file[1024];
	strcpy(file, fakeroot);
	strcat(file, filename);
	SHA_CTX c;
	FILE * inf;
	unsigned char * md = malloc(sizeof(unsigned char) * 20);
	char * ret = malloc(sizeof(unsigned char) * 41);
	memset(ret, 0, 41);
	char buffer[512];

	inf = fopen(file, "r");
	if (inf) {
		SHA1_Init(&c);
		while (!feof(inf)) {
			memset(buffer, 0, 512);
			fgets(buffer, 512, inf);
			SHA1_Update(&c, buffer, strlen(buffer));
		}

		SHA1_Final(md, &c);
		fclose(inf);
	}
	int i;
	for ( i = 0; i < 20; ++i) {
		char tmp[2];
		sprintf(tmp, "%02x", md[i]);
		strcat(ret, tmp);
	}
	return ret;
} //@}

void db_init(char * dbname)
{ //@{
	int rc;
	char * zErrMsg = 0;
	rc = sqlite3_open(dbname, &db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
} //@}

void db_close()
{ //@{
    sqlite3_close(db);
} //@}

int db_addfile(char * path, int pkgid, char * fakeroot)
{ //@{
	if (db) {
		int rc;
		char * zErrMsg = 0;
		char query[1024];

		strcpy(query, "INSERT INTO files VALUES('");
		strcat(query, path);
		strcat(query, "',");
		strcat(query, itoa(pkgid, 10));
		strcat(query, ",'");
		strcat(query, sha1(path, fakeroot));
		strcat(query, "')");
		rc = sqlite3_exec(db, query, NULL, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\nquery: %s\n", zErrMsg, query);
			sqlite3_free(zErrMsg);
		}
		printf("Added file: %s\n", path);
		return 0;
	}
	return -1;
} //@}

static int pkgid_callback(void * pkgid, int argc, char **argv, char **azColName)
{ //@{
	if (argc > 0) {
		*((int *)pkgid) = atoi(argv[0]);
	}
	return 0;
} //@}

int db_getpkgid(char * name, char * version)
{ //@{
	if (db) {
		int rc;
		char * zErrMsg = 0;
		char query[1024];

		strcpy(query, "SELECT id FROM  pkgs WHERE name='");
		strcat(query, name);
		strcat(query, "' AND version='");
		strcat(query, version);
		strcat(query, "' LIMIT 1");

		int pkgid = 0;
		rc = sqlite3_exec(db, query, pkgid_callback, &pkgid, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\nquery: %s\n", zErrMsg, query);
			sqlite3_free(zErrMsg);
		}


		if (pkgid == 0) {
			memset(query, 0, 1024);
			strcpy(query, "INSERT INTO pkgs(name, version) VALUES('");
			strcat(query, name);
			strcat(query, "','");
			strcat(query, version);
			strcat(query, "')");

			//printf("Q: %s\n", query);

			rc = sqlite3_exec(db, query, pkgid_callback, &pkgid, &zErrMsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "SQL error: %s\nquery: %s\n", zErrMsg, query);
				sqlite3_free(zErrMsg);
			}
			pkgid = sqlite3_last_insert_rowid(db);
		}

		return pkgid;
	} else {
		return -1;
	}
} //@}
