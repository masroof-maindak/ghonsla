#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdlib.h>

struct table_info {
	char *table;
	size_t idx;
	size_t size;
};

struct fs_settings {
	char *fsName;
	size_t fsSize;
	size_t numEntries;
	size_t blockSize;
	size_t fBlocks;
	size_t fNameLen;
};

struct dir_entry {
	bool valid;
	bool isDir;
	ushort nameLen;
	ushort parentNameLen;
	char *name;
	char *parentDir;
	size_t size;
	size_t firstBlockNo;
};

struct fat_entry {};

void write_entry_to_buf(const struct dir_entry *e, char *b, size_t *s);
bool init_fs(const struct fs_settings *fss);
struct fs_settings load_config();

#endif // FILESYSTEM_H
