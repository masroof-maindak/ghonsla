#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

#define IDX_NA SIZE_MAX
/*
 * NOTE: Most directory table modifications can be made via two possible
 * 'primary keys' so to speak. One being the index of an entry in the table, and
 * the other being the name+parent. Thus, relevant functions, take a parameter
 * 'i' for the case where the caller can afford to provide the former. If this
 * index is unknown however, the user should pass IDX_NA for the argument.
 */

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
bool open_fs(const struct filesystem_settings *fss);
void load_config(struct filesystem_settings *fss);

#endif // FILESYSTEM_H
