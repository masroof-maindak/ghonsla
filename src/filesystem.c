#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/defaults.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

extern FILE *fs;
extern char *optarg;
extern int optind;

size_t freeListPtr; /* idx of first block in the free chain */
#define INCREMENT_FREE_LIST_PTR                                                \
	do {                                                                       \
		if (freeListPtr != SIZE_MAX)                                           \
			freeListPtr = fat->blocks[freeListPtr].next;                       \
	} while (0)

/**
 * @brief creates or opens an existing filesystem file
 */
bool open_fs(struct fs_settings *fss, int argc, fs_table *dt, fs_table *fat) {
	if ((fs = fopen(FS_NAME, "r+")) == NULL) {
		if (errno == ENOENT) {
			/* doesn't exist */
			return true;
		} else {
			perror("fopen() in init_fs()");
			return false;
		}
	}

	if (argc > 1)
		printf("Disk file found, ignoring args\n");

	/* TODO: load fat, dt, and user settings from disk */
	//---
	*fss	 = DEFAULT_CFG;
	dt->size = fat->size = 0;
	dt->dirs			 = NULL;
	fat->blocks			 = NULL;
	//---

	return true;
}

/**
 * @brief gets the size of an entry in a directory table
 */
int get_dte_len(const dir_entry *dte) {
	return (sizeof(dir_entry) - sizeof(char *) + dte->nameLen);
}

/**
 * @brief return the index of an entry in a directory table
 *
 * @return SIZE_MAX if the name being searched for was not found
 */
size_t get_index_of_dir_entry(const char *name, size_t cwd,
							  const fs_table *dt) {
	unsigned short nameLen = strlen(name);

	for (size_t i = 0; i < dt->size; i++)
		if (dt->dirs[i].valid && dt->dirs[i].nameLen == nameLen &&
			dt->dirs[i].parentIdx == cwd &&
			strncmp(name, dt->dirs[i].name, nameLen) == 0)
			return i;

	return SIZE_MAX;
}

/**
 * @brief creates a new file or directory under the parent directory at
 * `cwd` index, if a free entry is found. `name` must point to a
 * heap-allocated string.
 */
bool create_dir_entry(char *name, size_t cwd, bool isDir, const fs_table *dt) {
	/* find free spot */
	size_t i = 1;
	for (; i < dt->size; i++)
		if (!dt->dirs[i].valid)
			break;

	size_t nameLen = strlen(name);

	if (i == dt->size || nameLen > MAX_NAME_LEN)
		return false;

	dt->dirs[i] = (dir_entry){.valid		 = true,
							  .isDir		 = isDir,
							  .nameLen		 = nameLen,
							  .name			 = name,
							  .size			 = 0,
							  .parentIdx	 = cwd,
							  .firstBlockIdx = SIZE_MAX};

	return true;
}

/**
 * @details read the contents of a file into a buffer, starting from a specified
 * index, and running till a specific length
 *
 * @pre buf has 'size' bytes
 *
 * @param fp start reading at this index
 * @param size read this many bytes
 */
int read_file_at(size_t i, char *const buf, size_t size,
				 struct fs_settings *fss, size_t fp, const fs_table *dt,
				 const fs_table *fat) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return -1;

	if (fp + size > dt->dirs[i].size)
		return -2;

	if (buf == NULL)
		return 0;

	size_t bIdx = dt->dirs[i].firstBlockIdx;

	while (fp > fss->blockSize) {
		fp -= fss->blockSize;
		bIdx = fat->blocks[bIdx].next;
	}

	char bData[fss->blockSize];
	size_t written = 0;

	while (size > 0) {
		if (read_block(bIdx, fss->blockSize, bData) != 0)
			return -3;

		int bytesCopied = MIN(fss->blockSize - fp, size);
		memcpy(buf + written, bData + fp, bytesCopied);

		fp = 0;
		size -= bytesCopied;
		written += bytesCopied;

		if (size > 0) {
			bIdx = fat->blocks[bIdx].next;
			if (bIdx == SIZE_MAX) {
				fprintf(stderr, "read_file_at(): unexpected EoF reached");
				return -4;
			}
		}
	}

	return 0;
}

/**
 * @brief writes a buf of data to a file, at the specified file index, ensuring
 * the updation of all relevant metadata accordingly
 *
 * @details After validation and setting up (i.e variables & firstBlockIdx if
 * need be), we loop until the entire buffer has been written to the file.
 * 	1. Read block
 * 	2. Update block
 * 	3. Write back
 * 	4. Update size/usage
 * 	5. Update write index & remaining bytes
 * 	6. Update block chain & possibly freeListPtr
 *
 * @param i file's index in the directory table
 * @param size the size of the buffer
 * @param fp the position of the file at which to start writing
 *
 * @return 0 on success, negative on failure
 */
int write_file_at(size_t i, const char *buf, size_t size,
				  struct fs_settings *fss, size_t fp, const fs_table *dt,
				  const fs_table *fat) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return -1;

	if (fp > dt->dirs[i].size)
		return -2;

	if (buf == NULL || size == 0)
		return 0;

	size_t bIdx = dt->dirs[i].firstBlockIdx;

	while (fp > fss->blockSize) {
		fp -= fss->blockSize;
		bIdx = fat->blocks[bIdx].next;
	}

	if (bIdx == SIZE_MAX) { /* file is empty */
		if (freeListPtr == SIZE_MAX) {
			fprintf(stderr, ERR_NO_AVAILABLE_BLOCKS);
			return -3;
		}

		bIdx = dt->dirs[i].firstBlockIdx = freeListPtr;
		INCREMENT_FREE_LIST_PTR;
		fat->blocks[bIdx].next = SIZE_MAX;
	}

	char bData[fss->blockSize];

	while (size > 0) {
		if (read_block(bIdx, fss->blockSize, bData) != 0)
			return -4;

		int bytesCopied = MIN(fss->blockSize - fp, size);
		memcpy(bData + fp, buf, bytesCopied);

		if (write_block(bIdx, fss->blockSize, bData) != 0)
			return -5;

		size_t newUsage = fp + bytesCopied;
		if (newUsage > fat->blocks[bIdx].used) {
			dt->dirs[i].size += (newUsage - fat->blocks[bIdx].used);
			fat->blocks[bIdx].used = newUsage;
		}

		fp = 0;
		size -= bytesCopied;
		buf += bytesCopied;

		if (size > 0) {
			if (fat->blocks[bIdx].next != SIZE_MAX) {
				bIdx = fat->blocks[bIdx].next;
				continue;
			}

			if (dt->dirs[bIdx].size / fss->blockSize > fss->fMaxBlocks) {
				fprintf(stderr, ERR_FILE_MAX_BLOCKS, fss->fMaxBlocks);
				return -6;
			}

			if (freeListPtr == SIZE_MAX) {
				fprintf(stderr, ERR_NO_AVAILABLE_BLOCKS);
				return -7;
			}

			bIdx = fat->blocks[bIdx].next = freeListPtr;
			INCREMENT_FREE_LIST_PTR;
			fat->blocks[bIdx].next = SIZE_MAX;
		}
	}

	return 0;
}

int append_to_file(size_t i, const char *buf, size_t size,
				   struct fs_settings *fss, const fs_table *dt,
				   const fs_table *fat) {
	return write_file_at(i, buf, size, fss, dt->dirs[i].size, dt, fat);
}

/**
 * @brief print the contents of a directory to stdout.
 */
bool print_directory_contents(size_t i, fs_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || !dt->dirs[i].isDir)
		return false;

	for (size_t j = 0; j < dt->size; j++)
		if (dt->dirs[j].valid && dt->dirs[j].parentIdx == i)
			printf("%s%s%s\n", dt->dirs[j].isDir ? BOLD_BLUE : CYAN,
				   dt->dirs[j].name, RESET);

	return true;
}

/**
 * @brief remove the entire contents of a file, i.e make them 'available' for
 * other files' writes.
 *
 * @pre if a file is empty, it's 'firstBlockIdx' is SIZE_MAX.
 * @pre the final block of a file's content chain has it's 'next' set to
 * SIZE_MAX.
 */
bool truncate_file(size_t i, fs_table *dt, fs_table *fat) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return false;

	if (dt->dirs[i].size == 0)
		return true;

	size_t bIdx = dt->dirs[i].firstBlockIdx;
	size_t prev = bIdx;

	/* traverse the file's chain until we reach the final block */
	while (bIdx != SIZE_MAX) {
		fat->blocks[bIdx].used = 0;
		prev				   = bIdx;
		bIdx				   = fat->blocks[bIdx].next;
	}
	bIdx = prev;

	/* Point the end of this file's chain towards the free list */
	fat->blocks[bIdx].next = freeListPtr;

	/* Set the start of this (now empty) chain as the free list */
	freeListPtr = dt->dirs[i].firstBlockIdx;

	return true;
}

/**
 * @brief Delete a file or recursively, the contents of a directory. The
 * 'name' is freed.
 */
bool remove_dir_entry(size_t i, fs_table *dt, fs_table *fat) {
	if (i == SIZE_MAX || !dt->dirs[i].valid)
		return false;

	if (!dt->dirs[i].isDir) {
		truncate_file(i, dt, fat);
	} else {
		for (size_t j = 0; j < dt->size; j++)
			if (dt->dirs[j].valid && dt->dirs[j].parentIdx == i)
				remove_dir_entry(j, dt, fat);
	}

	free(dt->dirs[i].name);
	dt->dirs[i].name  = "";
	dt->dirs[i].valid = false;
	dt->dirs[i].size  = 0;

	return true;
}

/**
 * @brief renames an entry in the global directory table. The old 'name' is
 * freed
 */
bool rename_dir_entry(char *newName, size_t i, fs_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid)
		return false;

	free(dt->dirs[i].name);
	dt->dirs[i].name	= newName;
	dt->dirs[i].nameLen = strlen(newName);

	return true;
}

/**
 * @brief serialises all the entries of a table to a buffer
 *
 * @details this should be called after the user has written other
 * information to the disk first
 *
 * @return idx of the buffer after filling it up
 */
size_t dump_dir_table_to_buf(char *buf, size_t idx, const fs_table *dt) {
	//--- TODO: abstract out
	/* size_t cap = 0; */
	/**/
	/* for (size_t i = 0; i < dt->size; i++) */
	/* 	cap += get_dte_len(&dt->u.dirTEntries[i]); */
	/**/
	/* char *buf = malloc(cap); */
	/* if (buf == NULL) { */
	/* 	perror("malloc() in dump_entries_to_buf()"); */
	/* 	return 0; */
	/* } */
	//---

	for (size_t i = 0; i < dt->size; i++)
		write_dir_entry_to_buf(&dt->dirs[i], buf, &idx);

	return idx;
}

/**
 * @details writes a directory table entry to a buffer's i'th index. It is
 * the caller's responsibility to ensure that the buffer is large enough to
 * hold the entry. The index is incremented accordingly.
 */
void write_dir_entry_to_buf(const dir_entry *e, char *b, size_t *i) {
	memcpy(b + *i, &e->valid, sizeof(bool));
	*i += sizeof(bool);
	memcpy(b + *i, &e->isDir, sizeof(bool));
	*i += sizeof(bool);
	memcpy(b + *i, &e->nameLen, sizeof(unsigned short));
	*i += sizeof(unsigned short);
	memcpy(b + *i, e->name, e->nameLen);
	*i += e->nameLen;
	memcpy(b + *i, &e->size, sizeof(size_t));
	*i += sizeof(size_t);
	memcpy(b + *i, &e->parentIdx, sizeof(size_t));
	*i += sizeof(size_t);
	memcpy(b + *i, &e->firstBlockIdx, sizeof(size_t));
	*i += sizeof(size_t);
}

/**
 * @param nmb number of metadata blocks
 */
void clear_out_fat(size_t nmb, fs_table *fat) {
	/* zero out blocks holding metadata */
	memset(fat->blocks, 0, nmb * sizeof(fat_entry));

	/* initialise the FAT as a free list */
	for (size_t i = nmb; i < fat->size; i++)
		fat->blocks[i] = (fat_entry){.used = 0, .next = i + 1};

	/* point the end of the free chain to SIZE_MAX */
	fat->blocks[fat->size - 1].next = SIZE_MAX;

	/* initialise free chain */
	/* CHECK: off by one? */
	freeListPtr = nmb;
}

/**
 * @brief creates a directory table in memory, populates it with the root
 * entry and garbage entries
 */
bool init_new_dir_t(int entryCount, fs_table *dt) {
	dt->size = entryCount;
	dt->dirs = malloc(sizeof(dir_entry) * dt->size);

	if (dt->dirs == NULL) {
		perror("malloc() in init_new_dir_t()");
		return false;
	}

	dt->dirs[0] = DIR_TABLE_ROOT_ENTRY;
	for (size_t i = 1; i < dt->size; i++)
		dt->dirs[i] = DIR_TABLE_GARBAGE_ENTRY;

	return true;
}

/**
 * @brief allocates a new file allocation table having enough space for
 * every block in the filesystem
 *
 * @param nb number of blocks in the filesystem
 * @param nmb number of blocks used for metadata (i.e tables and other
 * information to persist on disk)
 */
bool init_new_fat(size_t nb, size_t nmb, fs_table *faT) {
	faT->size	= nb;
	faT->blocks = malloc(faT->size * sizeof(fat_entry));

	if (faT->blocks == NULL) {
		perror("malloc() in init_new_fat()");
		return false;
	}

	clear_out_fat(nmb, faT);
	return true;
}

/**
 * @details creates a filesystem file as per the settings defined in `fss`,
 * and initialises the global directory/file allocation tables. If a
 * filesystem already exists before this function is called, the caller
 * should perform all relevant cleanup first, namely freeing the global
 * structures.
 */
bool init_new_fs(const struct fs_settings *fss, fs_table *dt, fs_table *fat) {
	/* open file for writing */
	if ((fs = fopen(FS_NAME, "w+")) == NULL) {
		perror("fopen() in create_empty_fs()");
		return false;
	}

	/* create relevant tables in memory */
	if (!init_new_dir_t(fss->entryCount, dt))
		goto fclose;

	if (!init_new_fat(fss->numBlocks, fss->numMdBlocks, fat)) {
		free(dt->dirs);
		goto fclose;
	}

	/* write garbage blocks to disk */
	char *buf = calloc(fss->blockSize, sizeof(char));
	if (buf == NULL) {
		perror("calloc() in create_empty_fs()");
		free(dt->dirs);
		free(fat->blocks);
		goto fclose;
	}

	for (size_t i = 0; i < fss->numBlocks; i++) {
		if (write_block(i, fss->blockSize, buf) != 0) {
			fprintf(stderr, "create_empty_fs(): failed at block #%zd\n", i);
			free(dt->dirs);
			free(fat->blocks);
			free(buf);
			goto fclose;
		}
	}

	free(buf);
	return true;

fclose:
	if (fclose(fs) == EOF)
		perror("fclose() in create_empty_fs()");
	return false;
}

/**
 * @brief: resets state-relevant tables to make them available to write over
 */
void format_fs(struct fs_settings *fss, fs_table *dt, fs_table *fat) {
	for (size_t i = 1; i < dt->size; i++)
		remove_dir_entry(i, dt, fat);
	clear_out_fat(fss->numMdBlocks, fat);
}

bool calc_and_validate_block_num(struct fs_settings *fss) {
	fss->numBlocks = fss->size * 1024 * 1024 / fss->blockSize;

	const size_t dirTBytes = MAX_SIZE_DIR_ENTRY * fss->entryCount,
				 fatBytes  = sizeof(fat_entry) * fss->numBlocks;

	fss->numMdBlocks = ((fatBytes + dirTBytes) / fss->blockSize) + 1;

	if (fss->numMdBlocks > fss->numBlocks) {
		fprintf(stderr,
				"init_new_fs(): Configuration error - metadata size exceeds "
				"available filesystem space; please adjust "
				"filesystem size, block size, "
				"or directory entry count.\n");
		return false;
	}

	return true;
}

/**
 * @brief parse user args for filesystem creation. Unset or erroneous
 * arguments are defaulted.
 */
bool load_config(struct fs_settings *fss, int argc, char **argv) {
	int opt;
	*fss = DEFAULT_CFG;

	while ((opt = getopt(argc, argv, "m:n:s:b:")) != -1) {
		switch (opt) {
		case 'm':
			parse_and_set_ul(&fss->size, optarg);
			break;
		case 'n':
			parse_and_set_ul(&fss->entryCount, optarg);
			break;
		case 's':
			parse_and_set_ul(&fss->blockSize, optarg);
			break;
		case 'b':
			parse_and_set_ul(&fss->fMaxBlocks, optarg);
			break;
		default:
			fprintf(stderr,
					"Usage: %s [-m size-in-MBs] [-n entry-count]  [-s "
					"block-size] [-b file-max-block-count]\n",
					argv[0]);
			return false;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "Ignoring non-option argv-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	if (!calc_and_validate_block_num(fss))
		return false;

	return true;
}
