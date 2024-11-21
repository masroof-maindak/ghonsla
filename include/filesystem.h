#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	char *fsName;
	size_t fsSize;
	size_t numEntries;
	size_t blockSize;
	size_t fBlocks;
	size_t fNameLen;
} fs_settings;

typedef struct {
	bool valid;
	bool isDir;
	unsigned short nameLen;
	char *name;
	size_t size;
	size_t parentIdx;
	size_t firstBlockNum;
} dirT_entry;

typedef struct {
	size_t used;
	size_t nextBlockNum;
} fat_entry;

typedef struct {
	size_t size;
	union {
		dirT_entry *dirTEntries;
		fat_entry *fatEntries;
	} u;
} fs_table;

void write_entry_to_buf(const dirT_entry *e, char *b, size_t *s);
bool init_fs(const fs_settings *fss);
fs_settings load_config();

#endif // FILESYSTEM_H
