#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "db.h"

#define PATHLEN 512

static char * fakeroot_cache = NULL;

char * get_pkgname(char * pkgname)
{ //@{
	int i;
	char * name = malloc(sizeof(char) * PATHLEN);
	memset(name, 0, PATHLEN);

	if (!pkgname) {
		char * cwd = getcwd(NULL, 0);
		pkgname = strrchr(cwd, '/');
		pkgname++;
		free(cwd);
	}

	for (i = 0; i < PATHLEN; ++i) {
		if (pkgname[i] == '-' || pkgname[i] == '.')
			break;
		else
			name[i] = pkgname[i];
	}
	return name;
} //@}

char * get_pkgversion()
{ //@{
	int i;
	char * cwd = getcwd(NULL, 0);
	char * v = strrchr(cwd, '-');
	char * version = malloc(sizeof(char) * PATHLEN);
	memset(version, 0, PATHLEN);
	if (v) {
		v++;
		for (i = 0; i < PATHLEN; ++i) {
			if (isalpha(v[i]) && (v[i-1] == '.'))
				break;
			else
				version[i] = v[i];
		}
	} else {
		version = "";
	}
	free(cwd);

	return version;
} //@}

char *time_stamp()
{ //@{
	char *timestamp = (char *)malloc(sizeof(char) * 16);
	time_t ltime;
	ltime=time(NULL);
	struct tm *tm;
	tm=localtime(&ltime);

	sprintf(timestamp,"_%04d-%02d-%02d_%02d.%02d:%02d", tm->tm_year+1900, tm->tm_mon,
				tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return timestamp;
} //@}

char * get_fakeroot()
{ //@{
	if (fakeroot_cache == NULL) {
		char * fakeroot = malloc(sizeof(char) * PATHLEN);
		strcpy(fakeroot, "/var/local/ivepkg/fakeroot/");
		//strcpy(fakeroot, "/mnt/lfs/var/local/ivepkg/fakeroot/");
		strcat(fakeroot, get_pkgname(NULL));
		strcat(fakeroot, "-");
		strcat(fakeroot, get_pkgversion());

		mkdir(fakeroot, 0700);

		fakeroot_cache = fakeroot;

		return fakeroot;
	}
	return fakeroot_cache;
} //@}

void register_in_db(char * dir, int pkgid)
{ //@{
	// This is ugly and should have a cleanup
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	int i;
	if((dp = opendir(dir)) == NULL) {
		fprintf(stderr,"cannot open directory: %s\n", dir);
		return;
	}

	chdir(dir);
	while((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name,&statbuf);
		if(S_ISDIR(statbuf.st_mode)) {
			/* Found a directory, but ignore . and .. */
			if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
				continue;
			/* Recurse at a new indent level */
			register_in_db(entry->d_name, pkgid);
		} else {
			char path[PATHLEN];
			char tmppath[PATHLEN];
			char * fakeroot = get_fakeroot();
			strcpy(path, getcwd(NULL,0));
			strcat(path, "/");
			strcat(path, entry->d_name);
			if (path[0] != '/') {
				strcpy(tmppath, path);
				strcpy(path, fakeroot);
				strcat(path, "/");
				strcat(path, tmppath);
			}
			strcpy(tmppath, path);
			size_t strlenfakeroot = strlen(fakeroot);
			for (i = strlenfakeroot; i < PATHLEN; ++i)
				path[i-strlenfakeroot] = tmppath[i];
			if (db_addfile(path, pkgid, fakeroot))
				printf("Unable to register %s\n", path);
		}
	}
	chdir("..");
	closedir(dp);
} //@}

int install_to_fakeroot(char * fakeroot)
{ //@{
	char install_cmd[4096];
	strcat(install_cmd, "make DESTDIR=");
	strcat(install_cmd, fakeroot);
	strcat(install_cmd, " install");
	printf("INSTALL CMD: %s\n", install_cmd);
	system(install_cmd);
	return 0;
} //@}

int install_to_root(char * fakeroot)
{ //@{
	chdir(fakeroot);
	system("tar cf - . | (cd / ; tar xf - )");
	return 0;
} //@}

int get_pkgid(char * name, char * version)
{ //@{
	return db_getpkgid(name, version);
} //@}

char * get_fileext(char * file)
{ //@{
	int i;
	char * ext = calloc(sizeof(char) * 16, 0);
	char * v = strrchr(file, '.');
	if (v) {
		v++;
		for (i = 0; i < 16; i++) {
			ext[i] = v[i];
		}
	}
	return ext;
} //@}

void unpack(int argc, char ** argv)
{ //@{
	int i;
	for (i = 2; i < argc; ++i) {
		char * filext;
		char buffer[512];
		memset(buffer, 0, 512);
		filext = get_fileext(argv[i]);
		printf("Fext: %s\n", filext);
		strcpy(buffer, "tar");
		if (strcmp(filext, "gz") == 0)
			strcat(buffer, " xfvz ");
		else if (strcmp(filext, "bz2") == 0)
			strcat(buffer, " xfvj ");
		else if (strcmp(filext, "xz") == 0)
			strcat(buffer, " xfvJ ");
		else
			return;
		strcat(buffer, argv[i]);
		system(buffer);
	}
} //@}

void usage()
{ //@{
	printf("Usage: ivepkg <command>\n\n \
Commands:\n\
    root 	    Install from fakeroot to root\n\
    register        Register a fakeroot's content to the database.\n\
    fakeroot        Install to fakeroot\n\
    unpack          Unpack a tarball\n\
    install         Install to fakeroot, register, install to root\n\
");
} //@}

int main(int argc, char ** argv)
{ //@{
	if (argc < 2) {
		usage();
		exit(0);
	}

	printf("Fakeroot: %s\n", get_fakeroot());
	printf("Package name: %s\n", get_pkgname(argv[2]));
	printf("Package version: %s\n", get_pkgversion());

	db_init("/var/local/ivepkg/ivepkg_db.sqlite3");
	if (strcmp(argv[1], "register") == 0) {
		printf("Register...\n");
		int pkgid = get_pkgid(get_pkgname(NULL), get_pkgversion());
		register_in_db(get_fakeroot(), pkgid);
	} else if (strcmp(argv[1], "fakeroot") == 0) {
		printf("Installing to fakeroot...\n");
		install_to_fakeroot(get_fakeroot());
	} else if (strcmp(argv[1], "root") == 0) {
		printf("Installing to root...\n");
		install_to_root(get_fakeroot());
	} else if (strcmp(argv[1], "install") == 0) {
		printf("Installing...\n");
		//Fakeroot
		char * fakeroot = get_fakeroot();
		install_to_fakeroot(fakeroot);
		//Register
		int pkgid = get_pkgid(get_pkgname(NULL), get_pkgversion());
		register_in_db(fakeroot, pkgid);
		//Root
		install_to_root(fakeroot);
	} else if (strcmp(argv[1], "unpack") == 0) {
		printf("Unpacking...\n");
		if (argc < 3) {
			usage();
			exit(0);
		}
		unpack(argc, argv);
	} else
		usage();

	db_close();
	return 0;
} //@}
