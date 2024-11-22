#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

struct filesystem_settings {
	size_t size;
	size_t entryCount;
	size_t blockSize;
	size_t fBlocks;
};

typedef struct {
	bool valid;
	bool isDir;
	unsigned short nameLen;
	char *name;
	size_t size;
	size_t parentIdx;
	size_t firstBlockIdx;
} dir_entry;

typedef struct {
	size_t used;
	size_t nextBlockNum;
} fat_entry;

typedef struct {
	size_t size;
	union {
		dir_entry *dirs;
		fat_entry *blocks;
	};
} fs_table;

void write_dir_entry_to_buf(const dir_entry *e, char *b, size_t *s);
bool init_fs(const struct filesystem_settings *fss);
struct filesystem_settings load_config();

#endif // FILESYSTEM_H
