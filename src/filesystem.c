#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/bool.h"
#include "../include/defaults.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

FILE *fs		  = NULL;
fs_table dirTable = {.size = 0, .dirs = NULL};
fs_table faT	  = {.size = 0, .blocks = NULL};
/* CHECK: don't give these global scope? */

int get_dte_len(const dir_entry *dte) {
	return (sizeof(dir_entry) - sizeof(char *) + dte->nameLen);
}

/**
 * @brief return the index of an entry in the global directory table
 *
 * @return SIZE_MAX if the name being searched for was not found
 */
size_t get_index_of_dir_entry(const char *name, size_t cwd) {
	unsigned short nameLen = strlen(name);

	for (size_t i = 0; i < dirTable.size; i++)
		if (dirTable.dirs[i].valid && dirTable.dirs[i].nameLen == nameLen &&
			dirTable.dirs[i].parentIdx == cwd &&
			strncmp(name, dirTable.dirs[i].name, nameLen) == 0)
			return i;

	return SIZE_MAX;
}

/**
 * @brief creates a new file or directory under the parent directory at `cwd`
 * index, if a free entry is found. `name` must point to a heap-allocated
 * string.
 */
bool create_dir_entry(char *name, size_t cwd, bool isDir) {
	size_t i = 1, nameLen = strlen(name);
	for (; i < dirTable.size; i++)
		if (!dirTable.dirs[i].valid)
			break;

	if (i == dirTable.size || nameLen > MAX_NAME_LEN)
		return false;

	dirTable.dirs[i] = (dir_entry){.valid		  = true,
								   .isDir		  = isDir,
								   .nameLen		  = nameLen,
								   .name		  = name,
								   .size		  = 0,
								   .parentIdx	  = cwd,
								   .firstBlockIdx = SIZE_MAX};

	return true;
}

bool append_to_file(size_t i, char *buf, struct filesystem_settings *fss) {
	if (i == SIZE_MAX || !dirTable.dirs[i].valid)
		return false;

	/*
	 * TODO: Append to file
	 * Determine final block of this file
	 *
	 * Determine number of blocks this buffer will take accounting for the space
	 * remaining in the final block
	 *
	 * 'Reclaim' the content of the final block if need be
	 *
	 * Write blocks in a loop, knowing which blocks to write from the 'next'
	 * field in a struct of fat_entry
	 */

	return true;
}

/**
 * @brief print the contents of a directory to stdout.
 */
bool print_directory_contents(size_t i) {
	if (i == SIZE_MAX || !dirTable.dirs[i].valid || !dirTable.dirs[i].isDir)
		return false;

	for (size_t j = 0; j < dirTable.size; j++)
		if (dirTable.dirs[j].valid && dirTable.dirs[j].parentIdx == i)
			printf("%s%s%s\n", dirTable.dirs[j].isDir ? BOLD_BLUE : CYAN,
				   dirTable.dirs[j].name, RESET);

	return true;
}

/**
 * @brief remove the entire contents of a file, i.e make them 'available' for
 * other files' writes. Note that the final block of a file's content
 * chain must have it's 'next' set to SIZE_MAX.
 */
bool truncate_file(size_t i) {
	if (i == SIZE_MAX || !dirTable.dirs[i].valid)
		return false;

	size_t blockNum = dirTable.dirs[i].firstBlockIdx, save = blockNum;

	while (blockNum != SIZE_MAX) {
		faT.blocks[blockNum].used = 0;
		blockNum				  = faT.blocks[blockNum].next;
	}

	/*
	 * TODO: point the last block in a file's content's chain to the 'first
	 * free' and set first free to point to the original start of this file
	 */

	return true;
}

/**
 * @brief Delete a file or recursively, the contents of a directory. The 'name'
 * is freed.
 */
bool remove_dir_entry(size_t i) {
	if (i == SIZE_MAX || !dirTable.dirs[i].valid)
		return false;

	if (!dirTable.dirs[i].isDir) {
		truncate_file(i);
	} else {
		for (size_t j = 0; j < dirTable.size; j++)
			if (dirTable.dirs[j].valid && dirTable.dirs[j].parentIdx == i)
				remove_dir_entry(j);
	}

	free(dirTable.dirs[i].name);
	dirTable.dirs[i].name  = "";
	dirTable.dirs[i].valid = false;

	return true;
}

/**
 * @brief renames an entry in the global directory table. The old 'name' is
 * freed
 */
bool rename_dir_entry(char *newName, size_t i) {
	if (i == SIZE_MAX || !dirTable.dirs[i].valid)
		return false;

	free(dirTable.dirs[i].name);
	dirTable.dirs[i].name	 = newName;
	dirTable.dirs[i].nameLen = strlen(newName);

	return true;
}

/**
 * @brief serialises all the entries of a table to a buffer
 *
 * @details this should be called after the user has written other information
 * to the disk first
 *
 * @return idx of the buffer after filling it up
 */
size_t dump_dir_table_to_buf(char *buf, size_t idx) {
	// --- TODO: abstract out
	/* size_t cap = 0; */
	/**/
	/* for (size_t i = 0; i < dirT.size; i++) */
	/* 	cap += get_dte_len(&dirT.u.dirTEntries[i]); */
	/**/
	/* char *buf = malloc(cap); */
	/* if (buf == NULL) { */
	/* 	perror("malloc() in dump_entries_to_buf()"); */
	/* 	return 0; */
	/* } */
	// ---

	for (size_t i = 0; i < dirTable.size; i++)
		write_dir_entry_to_buf(&dirTable.dirs[i], buf, &idx);

	return idx;
}

/**
 * @brief creates a directory table in memory, populates it with the root entry
 * and garbage entries
 */
void init_new_dir_t(int entryCount, fs_table *dirT) {
	dirT->size = entryCount;
	dirT->dirs = malloc(sizeof(dir_entry) * dirT->size);

	if (dirT->dirs == NULL) {
		perror("malloc() in init_new_dir_t()");
		return;
	}

	dirT->dirs[0] = DIR_TABLE_ROOT_ENTRY;
	for (size_t i = 1; i < dirT->size; i++)
		dirT->dirs[i] = DIR_TABLE_GARBAGE_ENTRY;
}

void clear_out_fat(size_t nmb, fs_table *faT) {
	/* zero out blocks holding metadata */
	memset(faT->blocks, 0, nmb * sizeof(fat_entry));

	/* initialise the FAT as a free list */
	for (size_t i = nmb; i < faT->size; i++)
		faT->blocks[i] = (fat_entry){.used = 0, .next = i + 1};

	/* any chain's final block points to SIZE_MAX */
	faT->blocks[faT->size - 1].next = SIZE_MAX;
}

/**
 * @brief allocates a new file allocation table having enough space for every
 * block in the filesystem
 *
 * @param nb number of blocks in the filesystem
 * @param nmb number of blocks used for metadata (i.e tables and other
 * information to persist on disk)
 */
void init_new_fat(size_t nb, size_t nmb, fs_table *faT) {
	faT->size	= nb;
	faT->blocks = malloc(faT->size * sizeof(fat_entry));

	if (faT->blocks == NULL) {
		perror("malloc() in init_new_fat()");
		return;
	}

	clear_out_fat(nmb, faT);
}

/**
 * @details creates a filesystem file as per the settings defined in `fss`, and
 * initialises the global directory/file allocation tables. If a filesystem
 * already exists before this function is called, the caller should perform all
 * relevant cleanup first, namely freeing the global structures.
 */
bool init_new_fs(const struct filesystem_settings *fss) {
	/* open file for writing */
	if ((fs = fopen(FS_NAME, "w+")) == NULL) {
		perror("fopen() in create_empty_fs()");
		return false;
	}

	/* create relevant tables in memory */
	init_new_dir_t(fss->entryCount, &dirTable);
	if (dirTable.dirs == NULL)
		goto fclose;

	init_new_fat(fss->numBlocks, fss->numMdBlocks, &faT);
	if (faT.blocks == NULL) {
		free(dirTable.dirs);
		goto fclose;
	}

	/* TODO: first free = numMetadataBlocks; */

	/* write garbage blocks to disk */
	char *buf = calloc(fss->blockSize, sizeof(char));
	if (buf == NULL) {
		perror("calloc() in create_empty_fs()");
		free(dirTable.dirs);
		free(faT.blocks);
		goto fclose;
	}

	for (size_t i = 0; i < fss->numBlocks; i++) {
		if (write_block(i, fss->blockSize, buf) != 0) {
			fprintf(stderr, "create_empty_fs(): failed at block #%ld\n", i);
			free(dirTable.dirs);
			free(faT.blocks);
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
 * @brief: clears state-relevant tables
 */
void quick_format_fs(struct filesystem_settings *fss) {
	for (size_t i = 1; i < dirTable.size; i++)
		remove_dir_entry(i);
	clear_out_fat(fss->numMdBlocks, &faT);
}

bool calc_and_validate_block_no(struct filesystem_settings *fss) {
	fss->numBlocks = fss->size * 1024 * 1024 / fss->blockSize;

	const size_t dirTBytes = MAX_SIZE_DIR_ENTRY * fss->entryCount,
				 fatBytes  = sizeof(fat_entry) * fss->numBlocks;

	fss->numMdBlocks = ((fatBytes + dirTBytes) / fss->blockSize) + 1;

	if (fss->numMdBlocks > fss->numBlocks) {
		fprintf(stderr,
				"init_new_fs(): Configuration error - metadata size exceeds "
				"available filesystem space. Please adjust "
				"filesystem size, block size, "
				"or directory entry count.\n");
		return false;
	}

	return true;
}

/**
 * @brief parse user args for filesystem creation. Unset or erroneous arguments
 * are defaulted.
 */
bool load_config(struct filesystem_settings *fss, int argc, char **argv) {
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
			parse_and_set_ul(&fss->fBlocks, optarg);
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

	if (!calc_and_validate_block_no(fss))
		return false;

	return true;
}

void tests(struct filesystem_settings *fss);

/**
 * @details opens the filesystem file if it exists, or creates a new one if not,
 */
bool init(struct filesystem_settings *fss, int argc, char **argv) {
	switch (open_fs(fss, argc)) {
	case 0: /* failed to open file */
		return false;
	case 1: /* file not present */
		if (!load_config(fss, argc, argv) || !init_new_fs(fss))
			return false;
		break;
	case 2: /* file present and loaded */
		break;
	}

	return true;
}

int main(int argc, char **argv) {
	int ret = 0;
	struct filesystem_settings fss;

	if (!init(&fss, argc, argv))
		return 1;

	/* TODO: ncurses menu loop */

	tests(&fss);

	/* cleanup: */
	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	free(dirTable.dirs);
	free(faT.blocks);
	return ret;
}

/**
 * @details writes a directory table entry to a buffer's i'th index. It is the
 * caller's responsibility to ensure that the buffer is large enough to hold the
 * entry. The index is incremented accordingly.
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
 * @brief creates or opens an existing filesystem file
 */
bool open_fs(struct filesystem_settings *fss, int argc) {
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

	/* TODO: load fat, dirTable, and user settings from disk */

	return true;
}

void tests(struct filesystem_settings *fss) {
	char *firstDir	= copy_string("firstDir");
	char *f1name	= copy_string("f1");
	char *f2name	= copy_string("f2");
	char *f3name	= copy_string("f3");
	char *rename	= copy_string("f2_renamed");
	char *secondDir = copy_string("secondDir");
	char *f4name	= copy_string("f4");
	size_t idx		= ROOT_IDX;

	create_dir_entry(firstDir, ROOT_IDX, true);
	create_dir_entry(f1name, ROOT_IDX, false);

	idx = get_index_of_dir_entry(f1name, ROOT_IDX);
	remove_dir_entry(idx);

	create_dir_entry(f2name, 1, false);
	create_dir_entry(f3name, 1, false);

	idx = get_index_of_dir_entry(f2name, 1);
	rename_dir_entry(rename, idx);

	printf("firstDir\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX);
	print_directory_contents(idx);
	create_dir_entry(secondDir, 1, true);
	puts("");

	idx = get_index_of_dir_entry(secondDir, 1);
	create_dir_entry(f4name, idx, false);
	printf("secondDir\n");
	print_directory_contents(idx);
	puts("");

	printf("firstDir\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX);
	print_directory_contents(idx);
	puts("");

	printf("/:\n");
	print_directory_contents(ROOT_IDX);
	puts("");

	quick_format_fs(fss);

	printf("/:\n");
	print_directory_contents(ROOT_IDX);

	/* free(firstDir); */
	/* free(f3name); */
	/* free(rename); */
	/* free(secondDir); */
	/* free(f4name); */
}
