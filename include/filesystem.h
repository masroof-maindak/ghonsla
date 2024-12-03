#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdlib.h>

#include "../include/bool.h"
#include "../include/defaults.h"

#define ROOT_IDX 0

#define ERR_NO_AVAILABLE_BLOCKS                                                \
	"write_to_file(): insufficient blocks available to complete write; "       \
	"remove data and try again\n"

#define ERR_FILE_MAX_BLOCKS                                                    \
	"write_to_file(): file has reached the maximum allowable number of "       \
	"blocks (%zu)\n"

struct fs_settings {
	/* Configurable; determined via CLI args */

	size_t size;	   /* filesystem size (in MBs) */
	size_t entryCount; /* number of directory entries */
	size_t blockSize;  /* size of one block */
	size_t fMaxBlocks; /* max no. of blocks in one file */

	/* Locked; determined at run-time based on the above */

	size_t numBlocks;	/* number of blocks in the filesystem's file */
	size_t numMdBlocks; /* number of blocks used to hold metadata */
};

typedef struct {
	bool valid;				/* entry holds a file or directory currently */
	bool isDir;				/* entry is a directory */
	unsigned short nameLen; /* name's length */
	char *name;				/* dir/file's name */
	size_t size;			/* number of bytes a file occupies */
	size_t parentIdx;		/* the index of the dir this entry is located in */
	uint32_t idx[MAX_INDEX_BLOCKS]; /* file's index */
} dir_entry;

typedef struct {
	size_t size;
	dir_entry *dirs;
} dir_table;

/* persistence */
bool deserialise_metadata(struct fs_settings *const fss, dir_table *const dt);
bool serialise_metadata(const struct fs_settings *fss,
						const dir_table *const dt);
bool obtain_dir_entry_from_buf(dir_entry *const e, const char *const b,
							   size_t *const i);
void write_dir_entry_to_buf(const dir_entry *const e, char *const b, size_t *i);

/* partition management */
bool init_new_dir_t(int entryCount, dir_table *dt, size_t numMdBlocks);
bool init_new_fs(const struct fs_settings *fss, dir_table *dt);
void format_fs(struct fs_settings *fss, dir_table *dt);

/* directory-table generic */
size_t get_index_of_dir_entry(const char *name, size_t cwd,
							  const dir_table *dt);
bool create_dir_entry(char *name, size_t cwd, bool isDir, const dir_table *dt);
bool remove_dir_entry(size_t i, dir_table *dt);
bool rename_dir_entry(char *newName, size_t i, dir_table *dt);

/* file-specific */
bool truncate_file(size_t i, dir_table *dt);
int read_file_at(size_t i, char *const buf, size_t size,
				 struct fs_settings *fss, size_t fPos, const dir_table *dt);
int write_to_file(size_t i, const char *buf, size_t size,
				  const struct fs_settings *fss, size_t fPos,
				  const dir_table *dt);
int append_to_file(size_t i, const char *buf, size_t size,
				   struct fs_settings *fss, const dir_table *dt);

/* directory-specific */
dir_entry **get_directory_entries(size_t i, const dir_table *const dt,
								  size_t *n);
void print_directory_contents(size_t i, const dir_table *const dt);

/* fs_settings */
bool parse_config_args(struct fs_settings *fss, int argc, char **argv);
bool compute_and_check_block_counts(struct fs_settings *const fss);

#endif // FILESYSTEM_H
