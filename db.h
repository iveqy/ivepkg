#ifndef DB_H
#define DB_H

int db_addfile(char * path, int pkgid, char * fakeroot);
int db_getpkgid(char * name, char * version);
void db_init(char * dbname);
void db_close();
#endif
