#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

#include "../include/bool.h"

#define ERR_NO_AVAILABLE_BLOCKS                                                \
	"write_to_file(): insufficient blocks available to complete write; "       \
	"remove data and try again\n"

#define ERR_FILE_MAX_BLOCKS                                                    \
	"write_to_file(): file has reached the maximum allowable number of "       \
	"blocks (%zd)\n"

struct fs_settings {
	/* Configurable; determined via CLI args */

	size_t size;	   /* filesystem size (in MBs) */
	size_t entryCount; /* number of directory entries */
	size_t blockSize;  /* size of one block */
	size_t fMaxBlocks; /* max no. of blocks in one file */

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

/* persistence */
size_t dump_dir_table_to_buf(char *buf, size_t idx, const fs_table *dt);
void write_dir_entry_to_buf(const dir_entry *e, char *b, size_t *s);
bool open_fs(struct fs_settings *fss, int argc, fs_table *dt, fs_table *fat);

/* partition management */
bool init_new_dir_t(int entryCount, fs_table *dt);
bool init_new_fat(size_t nb, size_t nmb, fs_table *fat);
bool init_new_fs(const struct fs_settings *fss, fs_table *dt, fs_table *fat);
void clear_out_fat(size_t nmb, fs_table *faT);
void format_fs(struct fs_settings *fss, fs_table *dt, fs_table *fat);

/* directory-table generic */
int get_size_of_dir_entry(const dir_entry *dte);
size_t get_index_of_dir_entry(const char *name, size_t cwd, const fs_table *dt);
bool create_dir_entry(char *name, size_t cwd, bool isDir, const fs_table *dt);
bool remove_dir_entry(size_t i, fs_table *dt, fs_table *fat);
bool rename_dir_entry(char *newName, size_t i, fs_table *dt);

/* file-specific */
bool truncate_file(size_t i, fs_table *dt, fs_table *fat);
int write_to_file(size_t i, const char *buf, size_t size,
				  struct fs_settings *fss, size_t fp, const fs_table *dt,
				  const fs_table *fat);
int append_to_file(size_t i, const char *buf, size_t size,
				   struct fs_settings *fss, const fs_table *dt,
				   const fs_table *fat);

/* directory-specific */
bool print_directory_contents(size_t i, fs_table *dt);

/* fs_settings */
bool load_config(struct fs_settings *fss, int argc, char **argv);
bool calc_and_validate_block_num(struct fs_settings *fss);

#endif // FILESYSTEM_H
