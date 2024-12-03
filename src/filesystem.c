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
 * @brief return the index of an entry in a directory table
 *
 * @return SIZE_MAX if the name being searched for was not found
 */
size_t get_index_of_dir_entry(const char *name, size_t cwd,
							  const dir_table *dt) {
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
bool create_dir_entry(char *name, size_t cwd, bool isDir, const dir_table *dt) {
	/* find free spot & and verify we don't exist already */
	if (get_index_of_dir_entry(name, cwd, dt) != SIZE_MAX)
		return false;

	size_t i = 1;
	for (; i < dt->size; i++)
		if (!dt->dirs[i].valid)
			break;

	size_t nameLen = strlen(name);

	if (i == dt->size || nameLen > MAX_NAME_LEN)
		return false;

	dt->dirs[i].valid	  = true;
	dt->dirs[i].isDir	  = isDir;
	dt->dirs[i].nameLen	  = nameLen;
	dt->dirs[i].name	  = name;
	dt->dirs[i].size	  = 0;
	dt->dirs[i].parentIdx = cwd;

	return true;
}

/**
 * @details read the contents of a file into a buffer, starting from a specified
 * index, and running till a specific length
 *
 * @pre retBuf is 'size' bytes long
 *
 * @param fp start reading at this index
 * @param size read this many bytes
 */
int read_file_at(size_t i, char *const retBuf, size_t size,
				 struct fs_settings *fss, size_t fPos, const dir_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return -1;

	if (fPos + size > dt->dirs[i].size)
		return -2;

	if (retBuf == NULL)
		return 0;

	char dataBuf[fss->blockSize];
	size_t written = 0;

	for (unsigned short blockNo = fPos / fss->blockSize; size > 0; blockNo++) {
		if (read_block(dt->dirs[i].idx[blockNo], fss->blockSize, dataBuf) != 0)
			return -3;

		int bytesCopied = MIN(fss->blockSize - fPos, size);
		memcpy(retBuf + written, dataBuf + fPos, bytesCopied);

		fPos = 0;
		size -= bytesCopied;
		written += bytesCopied;
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
int write_to_file(size_t i, const char *buf, size_t size,
				  const struct fs_settings *fss, size_t fPos,
				  const dir_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return -1;

	if (fPos > dt->dirs[i].size)
		return -2;

	if (buf == NULL || size == 0)
		return 0;

	char dataBuf[fss->blockSize];

	for (unsigned short blockNo = fPos / fss->blockSize; size > 0; blockNo++) {
		if (read_block(dt->dirs[i].idx[blockNo], fss->blockSize, dataBuf) != 0)
			return -3;

		int bytesCopied = MIN(fss->blockSize - fPos, size);
		memcpy(dataBuf + fPos, buf, bytesCopied);

		if (write_block(dt->dirs[i].idx[blockNo], fss->blockSize, dataBuf) != 0)
			return -4;

		/* update file size */
		if (blockNo == dt->dirs[i].size / fss->blockSize) {
			size_t finalBlockUsage = dt->dirs[i].size % fss->blockSize;
			size_t newUsage		   = fPos + bytesCopied;
			if (newUsage > finalBlockUsage)
				dt->dirs[i].size += newUsage;
		}

		fPos = 0;
		size -= bytesCopied;
		buf += bytesCopied;
	}

	return 0;
}

int append_to_file(size_t i, const char *buf, size_t size,
				   struct fs_settings *fss, const dir_table *dt) {
	return write_to_file(i, buf, size, fss, dt->dirs[i].size, dt);
}

/**
 * @brief return all the children of a provided directory in a null-terminated
 * array; the user must free after use.
 *
 * @param n number of children
 */
dir_entry **get_directory_entries(size_t i, const dir_table *const dt,
								  size_t *n) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || !dt->dirs[i].isDir)
		return NULL;

	*n				= 0;
	size_t numDirs	= 8;
	dir_entry **ret = malloc(numDirs * sizeof(*ret));

	if (ret == NULL) {
		perror("malloc() in get_directory_contents()");
		return NULL;
	}

	/* generate list */
	for (size_t j = 1; j < dt->size; j++) {
		if (dt->dirs[j].valid && dt->dirs[j].parentIdx == i) {
			ret[(*n)++] = &dt->dirs[j];

			if (*n >= numDirs) {
				numDirs *= 2;
				void *tmp = realloc(ret, numDirs);
				if (tmp == NULL) {
					perror("realloc() in get_directory_contents()");
					free(ret);
					return NULL;
				}
				ret = tmp;
			}
		}
	}

	ret[*n] = NULL;

	/* TODO: sort entries */

	return ret;
}

void print_directory_contents(size_t i, const dir_table *const dt) {
	size_t x;
	dir_entry **e = get_directory_entries(i, dt, &x);
	if (e == NULL)
		return;

	for (size_t i = 0; e[i] != NULL; i++)
		printf("%s%s%s\n", e[i]->isDir ? BOLD_BLUE : CYAN, e[i]->name, RESET);

	free(e);
}

/**
 * @brief remove the entire contents of a file, i.e make them 'available' for
 * other files' writes.
 *
 * @pre if a file is empty, it's 'firstBlockIdx' is SIZE_MAX.
 * @pre the final block of a file's content chain has it's 'next' set to
 * SIZE_MAX.
 */
bool truncate_file(size_t i, dir_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid || dt->dirs[i].isDir)
		return false;

	dt->dirs[i].size = 0;
	return true;
}

/**
 * @brief Delete a file or recursively, the contents of a directory. The
 * 'name' is freed.
 */
bool remove_dir_entry(size_t i, dir_table *dt) {
	if (i == SIZE_MAX || i == 0 || i == ROOT_IDX || !dt->dirs[i].valid)
		return false;

	if (!dt->dirs[i].isDir) {
		truncate_file(i, dt);
	} else {
		for (size_t j = 0; j < dt->size; j++)
			if (dt->dirs[j].valid && dt->dirs[j].parentIdx == i)
				remove_dir_entry(j, dt);
	}

	free(dt->dirs[i].name);
	dt->dirs[i] = DIR_TABLE_GARBAGE_ENTRY;
	return true;
}

/**
 * @brief renames an entry in the global directory table. The old 'name' is
 * freed. On failure, the user should free newName
 */
bool rename_dir_entry(char *newName, size_t i, dir_table *dt) {
	if (i == SIZE_MAX || !dt->dirs[i].valid)
		return false;

	/* if an entry w/ that name already exists */
	if (get_index_of_dir_entry(newName, dt->dirs[i].parentIdx, dt) != SIZE_MAX)
		return false;

	free(dt->dirs[i].name);
	dt->dirs[i].name	= newName;
	dt->dirs[i].nameLen = strlen(newName);

	return true;
}

/**
 * @brief writes all metadata to a buffer and then the filesystem
 */
bool serialise_metadata(const struct fs_settings *fss,
						const dir_table *const dt) {
	size_t size = 0;
	char buf[fss->numMdBlocks * fss->blockSize];
	memset(buf, 0, sizeof(buf));

	/* filesystem settings */
	memcpy(buf + size, fss, sizeof(struct fs_settings));
	size += sizeof(struct fs_settings);

	/* directory table */
	for (size_t i = 0; i < dt->size; i++)
		write_dir_entry_to_buf(&dt->dirs[i], buf, &size);

	/* serialisation */
	for (size_t i = 0; i < fss->numMdBlocks; size -= fss->blockSize, i++)
		if (write_block(i, fss->blockSize, buf + (i * fss->blockSize)) < 0)
			return false;

	return true;
}

/**
 * @brief reads a buf at the i'th index to obtain a directory table entry,
 * allocating space for names
 */
bool obtain_dir_entry_from_buf(dir_entry *const e, const char *const b,
							   size_t *const i) {
	memcpy(&e->valid, b + *i, sizeof(e->valid));
	*i += sizeof(e->valid);
	memcpy(&e->isDir, b + *i, sizeof(e->isDir));
	*i += sizeof(e->isDir);
	memcpy(&e->nameLen, b + *i, sizeof(e->nameLen));
	*i += sizeof(e->nameLen);

	if (e->nameLen == 0) {
		e->name = "";
	} else if (e->nameLen == 1 && b[*i] == '/') {
		e->name = "/";
	} else {
		e->name = malloc(e->nameLen + 1);
		if (e->name == NULL) {
			perror("malloc() in obtain_dir_entry_from_buf()");
			return false;
		}
		memcpy(e->name, b + *i, e->nameLen);
		e->name[e->nameLen] = '\0';
	}

	*i += e->nameLen;
	memcpy(&e->size, b + *i, sizeof(e->size));
	*i += sizeof(e->size);
	memcpy(&e->parentIdx, b + *i, sizeof(e->parentIdx));
	*i += sizeof(e->parentIdx);
	memcpy(&e->idx, b + *i, sizeof(e->idx));
	*i += sizeof(e->idx);

	return true;
}

/**
 * @brief recovers all metadata from the filesystem to the relevant structures
 */
bool deserialise_metadata(struct fs_settings *const fss, dir_table *const dt) {
	/* obtain settings */
	size_t size = sizeof(struct fs_settings);
	char tmp[BLOCK_SIZE];

	if (read_block(0, BLOCK_SIZE, tmp) < 0)
		return false;

	memcpy(fss, tmp, size);

	char buf[fss->numMdBlocks * fss->blockSize];
	memset(buf, 0, sizeof(buf));

	/* deserialise */
	for (size_t i = 0; i < fss->numMdBlocks; i++)
		if (read_block(i, fss->blockSize, buf + (i * fss->blockSize)) < 0)
			return false;

	/* directory table */
	dt->size = fss->entryCount;
	dt->dirs = malloc(sizeof(dir_entry) * fss->entryCount);

	if (dt->dirs == NULL) {
		perror("malloc() in deserialise_metadata() - dt->dirs");
		return false;
	}

	for (size_t i = 0; i < dt->size; i++)
		if (!obtain_dir_entry_from_buf(&dt->dirs[i], buf, &size))
			return false;

	return true;
}

/**
 * @details writes a directory table entry to a buffer's i'th index. The
 * index is incremented accordingly.
 *
 * @pre the buffer is large enough to hold the entry
 */
void write_dir_entry_to_buf(const dir_entry *const e, char *const b,
							size_t *i) {
	memcpy(b + *i, &e->valid, sizeof(e->valid));
	*i += sizeof(e->valid);
	memcpy(b + *i, &e->isDir, sizeof(e->isDir));
	*i += sizeof(e->isDir);
	memcpy(b + *i, &e->nameLen, sizeof(e->nameLen));
	*i += sizeof(e->nameLen);
	memcpy(b + *i, e->name, e->nameLen);
	*i += e->nameLen;
	memcpy(b + *i, &e->size, sizeof(e->size));
	*i += sizeof(e->size);
	memcpy(b + *i, &e->parentIdx, sizeof(e->parentIdx));
	*i += sizeof(e->parentIdx);
	memcpy(b + *i, &e->idx, sizeof(e->idx));
	*i += sizeof(e->idx);
}

/**
 * @brief creates a directory table in memory, populates it with the root
 * entry and garbage entries
 */
bool init_new_dir_t(int entryCount, dir_table *dt, size_t numMdBlocks) {
	dt->size = entryCount;
	dt->dirs = malloc(sizeof(*dt->dirs) * dt->size);

	if (dt->dirs == NULL) {
		perror("malloc() in init_new_dir_t()");
		return false;
	}

	size_t c = numMdBlocks;

	dt->dirs[ROOT_IDX] = DIR_TABLE_ROOT_ENTRY;
	for (int j = 0; j < MAX_INDEX_BLOCKS; j++, c++)
		dt->dirs[ROOT_IDX].idx[j] = c;

	for (size_t i = 1; i < dt->size; i++) {
		dt->dirs[i] = DIR_TABLE_GARBAGE_ENTRY;
		for (int j = 0; j < MAX_INDEX_BLOCKS; j++, c++) {
			dt->dirs[i].idx[j] = c;
		}
	}

	return true;
}

/**
 * @details creates a filesystem file as per the settings defined in `fss`,
 * and initialises the global directory/file allocation tables. If a
 * filesystem already exists before this function is called, the caller
 * should perform all relevant cleanup first, namely freeing the global
 * structures.
 */
bool init_new_fs(const struct fs_settings *fss, dir_table *dt) {
	/* open file for writing */
	if ((fs = fopen(FS_NAME, "w+")) == NULL) {
		perror("fopen() in init_new_fs()");
		return false;
	}

	/* create relevant tables in memory */
	if (!init_new_dir_t(fss->entryCount, dt, fss->numMdBlocks))
		goto fclose;

	/* write garbage blocks to disk */
	char *buf = calloc(fss->blockSize, sizeof(char));
	if (buf == NULL) {
		perror("calloc() in init_new_fs()");
		free(dt->dirs);
		goto fclose;
	}

	for (size_t i = 0; i < fss->numBlocks; i++) {
		if (write_block(i, fss->blockSize, buf) != 0) {
			fprintf(stderr, "init_new_fs(): failed at block #%zd\n", i);
			free(dt->dirs);
			free(buf);
			goto fclose;
		}
	}

	free(buf);
	return true;

fclose:
	if (fclose(fs) == EOF)
		perror("fclose() in init_new_fs()");
	return false;
}

/**
 * @brief: resets state-relevant tables to make them available to write over
 */
void format_fs(struct fs_settings *fss, dir_table *dt) {
	for (size_t i = 1; i < fss->entryCount; i++)
		remove_dir_entry(i, dt);
}

/**
 * @brief determines the number of blocks and metadata blocks from other
 * provided information and updates the settings accordingly
 */
bool compute_and_check_block_counts(struct fs_settings *const fss) {
	fss->numBlocks = fss->size * 1024 * 1024 / fss->blockSize;

	const size_t dirTBytes = MAX_SIZE_DIR_ENTRY * fss->entryCount,
				 stBytes   = sizeof(struct fs_settings);

	fss->numMdBlocks = ((dirTBytes + stBytes) / fss->blockSize) + 1;

	if (fss->numMdBlocks > fss->numBlocks) {
		fprintf(stderr,
				"init_new_fs(): Configuration error - metadata size exceeds "
				"available filesystem space; please adjust "
				"filesystem size, block size, "
				"or directory entry count.\n");
		return false;
	}

	/* TODO: check if final block in final index is out of bounds */

	return true;
}

/**
 * @brief parse user args for filesystem creation. Unset or erroneous
 * arguments are defaulted.
 */
bool parse_config_args(struct fs_settings *fss, int argc, char **argv) {
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

	if (!compute_and_check_block_counts(fss))
		return false;

	return true;
}
