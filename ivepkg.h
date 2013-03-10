#ifndef IVEPKG_H
#define IVEPKG_H

typedef struct file_tag file_entry;
struct file_tag {
	char * name;
	file_entry * next;
};

typedef struct pkg_tag pkg_entry;
struct pkg_tag {
	int id;
	pkg_entry * next;
};

typedef struct pkg_struct {
	int id;
	char * name;
	char * version;
	file_entry * file;
} pkg;
#endif
