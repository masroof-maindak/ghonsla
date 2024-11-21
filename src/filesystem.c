#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../include/bool.h"
#include "../include/defaults.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

int get_dte_len(const dirT_entry *dte) {
	return (sizeof(dirT_entry) - sizeof(char *) + dte->nameLen);
}

FILE *fs	  = NULL;
fs_table dirT = {.size = 0, .u.dirTEntries = NULL};
fs_table faT  = {.size = 0, .u.fatEntries = NULL};

/**
 * @brief return the index of an entry in the global directory table
 *
 * @return SIZE_MAX if the name being searched for was not found
 */
size_t get_index_of_dir_entry(const char *name, size_t cwd) {
	unsigned short nameLen = strlen(name);

	for (size_t i = 0; i < dirT.size; i++)
		if (dirT.u.dirTEntries[i].valid &&
			dirT.u.dirTEntries[i].nameLen == nameLen &&
			dirT.u.dirTEntries[i].parentIdx == cwd &&
			strncmp(name, dirT.u.dirTEntries[i].name, nameLen) == 0)
			return i;

	return SIZE_MAX;
}

/**
 * @brief creates a new file or directory under the parent directory at `cwd`
 * index. The `name` must point to a heap-allocated string.
 */
bool create_dir_entry(char *name, size_t cwd, bool isDir) {
	size_t i = 1;

	for (; i < dirT.size; i++)
		if (!dirT.u.dirTEntries[i].valid)
			break;

	if (i == dirT.size)
		return false;

	dirT.u.dirTEntries[i] = (dirT_entry){.valid			= true,
										 .isDir			= isDir,
										 .nameLen		= strlen(name),
										 .name			= name,
										 .size			= 0,
										 .parentIdx		= cwd,
										 .firstBlockNum = SIZE_MAX};

	return true;
}

bool append_to_file(char *buf, fs_settings *fss) {

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
 * @brief remove the entire contents of a file. Note that the final block of a
 * file's contents must have it's 'next' set to SIZE_MAX.
 *
 * @return the index of the file in the global directory table
 */
size_t truncate_file(const char *name, size_t cwd) {
	size_t i = get_index_of_dir_entry(name, cwd);
	if (i == SIZE_MAX)
		return SIZE_MAX;

	size_t blockNum = dirT.u.dirTEntries[i].firstBlockNum, save = blockNum;

	while (blockNum != SIZE_MAX) {
		faT.u.fatEntries[blockNum].used = 0;
		blockNum = faT.u.fatEntries[blockNum].nextBlockNum;
	}

	/*
	 * TODO: point the last block in a file's content's chain to the 'first
	 * free' and set first free to point to the original start of this file
	 */

	return i;
}

/**
 * @brief renames a file in the global directory table. The 'name' is freed
 */
bool remove_file(const char *name, size_t cwd) {
	size_t i = truncate_file(name, cwd);
	if (i == SIZE_MAX)
		return false;

	free(dirT.u.dirTEntries[i].name);
	dirT.u.dirTEntries[i].name			= "";
	dirT.u.dirTEntries[i].valid			= false;
	dirT.u.dirTEntries[i].firstBlockNum = SIZE_MAX;

	return true;
}

/**
 * @brief renames an entry in the global directory table. The old 'name' is
 * freed
 */
bool rename_dir_entry(char *name, char *newName, size_t cwd) {
	size_t i = get_index_of_dir_entry(name, cwd);
	if (i == SIZE_MAX)
		return false;

	free(dirT.u.dirTEntries[i].name);
	dirT.u.dirTEntries[i].name	  = newName;
	dirT.u.dirTEntries[i].nameLen = strlen(newName);

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
size_t dump_entries_to_buf(fs_table *t, char *buf, size_t idx) {
	// --- TOOD: abstract out
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

	for (size_t i = 0; i < t->size; i++)
		write_entry_to_buf(&t->u.dirTEntries[i], buf, &idx);

	return idx;
}

/**
 * @brief creates a directory table in memory, populates it with the root entry
 * and garbage entries
 */
void init_new_dir_t(int entryCount, fs_table *dirT) {
	dirT->size			= entryCount;
	dirT->u.dirTEntries = malloc(sizeof(dirT_entry) * dirT->size);

	if (dirT->u.dirTEntries == NULL) {
		perror("malloc() in init_new_dir_t()");
		return;
	}

	dirT->u.dirTEntries[0] = DIR_TABLE_ROOT_ENTRY;
	for (size_t i = 1; i < dirT->size; i++)
		dirT->u.dirTEntries[i] = DIR_TABLE_GARBAGE_ENTRY;
}

void init_new_fat(size_t numBlocks, size_t numMetadataBlocks, fs_table *faT) {
	faT->size		  = numBlocks - numMetadataBlocks;
	faT->u.fatEntries = malloc(sizeof(fat_entry) * faT->size);

	if (faT->u.fatEntries == NULL) {
		perror("malloc() in init_new_fat()");
		return;
	}

	/* initialise FAT as a free list */
	/* CHECK: off-by-one? */
	for (size_t i = 0; i < faT->size; i++)
		faT->u.fatEntries[i] =
			(fat_entry){.used = 0, .nextBlockNum = numMetadataBlocks + i + 1};

	faT->u.fatEntries[faT->size - 1].nextBlockNum = SIZE_MAX;
}

/**
 * @details creates a filesystem file as per the settings defined in `fss`, and
 * initialises the global directory/file allocation tables
 */
bool init_new_fs(const fs_settings *fss) {
	const size_t numBlocks = (fss->fsSize * 1024 * 1024) / fss->blockSize,
				 fatBytes  = sizeof(fat_entry) * numBlocks,
				 dirTBytes = MAX_SIZE_DIR_ENTRY * fss->numEntries,
				 numMetadataBlocks = (fatBytes + dirTBytes) / fss->blockSize;

	/* open file for writing */
	if ((fs = fopen(fss->fsName, "w+")) == NULL) {
		perror("fopen() in create_empty_fs()");
		return false;
	}

	/* create relevant tables in memory */
	init_new_dir_t(fss->numEntries, &dirT);
	if (dirT.u.dirTEntries == NULL)
		goto fclose;

	init_new_fat(numBlocks, numMetadataBlocks, &faT);
	if (faT.u.fatEntries == NULL) {
		free(dirT.u.dirTEntries);
		goto fclose;
	}

	/* TODO: first free = numMetadataBlocks; */

	/* write garbage blocks to disk */
	char *buf = calloc(fss->blockSize, sizeof(char));
	if (buf == NULL) {
		perror("calloc() in create_empty_fs()");
		free(dirT.u.dirTEntries);
		free(faT.u.fatEntries);
		goto fclose;
	}

	for (size_t i = 0; i < numBlocks; i++) {
		if (write_block(i, fss->blockSize, buf) != 0) {
			fprintf(stderr, "create_empty_fs(): failed at block #%ld\n", i);
			free(dirT.u.dirTEntries);
			free(faT.u.fatEntries);
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

int main(/* int argc, char** argv*/) {
	int ret = 0;

	/* TODO: menu - later: loop w/ ncurses */

	/* user settings */
	fs_settings userFsSettings = DEFAULT_CFG;

	/* init filesystem file */
	if (!init_fs(&userFsSettings))
		return 1;

	/* TESTING */
	char *d1name = copy_string("firstDir");
	char *f1name = copy_string("f1");
	char *f2name = copy_string("f2");
	char *f3name = copy_string("f3");
	char *rename = copy_string("f2_renamed");
	create_dir_entry(d1name, 0, true);
	create_dir_entry(f1name, 0, false);
	remove_file(f1name, 0);
	create_dir_entry(f2name, 1, false);
	create_dir_entry(f3name, 1, false);
	rename_dir_entry(f2name, rename, 1);
	free(d1name);
	free(f3name);
	free(rename);
	/* --------*/

	/* cleanup: */
	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	free(dirT.u.dirTEntries);
	free(faT.u.fatEntries);
	return ret;
}

/**
 * @details writes a directory table entry to a buffer's i'th index. It is the
 * caller's responsibility to ensure that the buffer is large enough to hold the
 * entry. The index is incremented accordingly.
 */
void write_entry_to_buf(const dirT_entry *e, char *b, size_t *i) {
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
	memcpy(b + *i, &e->firstBlockNum, sizeof(size_t));
	*i += sizeof(size_t);
}

/**
 * @brief creates or opens an existing filesystem file
 */
bool init_fs(const fs_settings *fss) {
	if ((fs = fopen(FS_NAME, "r+")) == NULL) {
		if (errno == ENOENT) {
			return init_new_fs(fss);
		} else {
			perror("fopen() in init_fs()");
			return false;
		}
	} else {
		/* TODO: load faT and dirT from disk */
	}

	return true;
}
