#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

#define ROOT_IDX 0

struct filesystem_settings {
	/* Configurable; determined via CLI args */

	size_t size;	   /* filesystem size (in MBs) */
	size_t entryCount; /* number of directory entries */
	size_t blockSize;  /* size of one block */
	size_t fBlocks;	   /* no. of blocks in one file */

	/* Locked; determined at run-time based on the above */

	size_t numBlocks;	/* number of blocks in the file */
	size_t numMdBlocks; /* number of blocks used to hold metadata */
};

typedef struct {
	bool valid;				/* entry holds a file or directory currently */
	bool isDir;				/* entry is a directory */
	unsigned short nameLen; /* name's length */
	char *name;				/* dir/file's name */
	size_t size;			/* number of bytes a file occupies */
	size_t parentIdx;		/* the index of the dir this entry is in */
	size_t firstBlockIdx;	/* the index of the first block holding this file's
							   content chain in the FAT */
} dir_entry;

typedef struct {
	size_t used; /* space used in this block */
	size_t next; /* index of next block */
} fat_entry;

typedef struct {
	size_t size;
	union {
		dir_entry *dirs;
		fat_entry *blocks;
	};
} fs_table;

void write_dir_entry_to_buf(const dir_entry *e, char *b, size_t *s);
bool open_fs(struct filesystem_settings *fss, int argc);
bool load_config(struct filesystem_settings *fss, int argc, char **argv);

#endif // FILESYSTEM_H
