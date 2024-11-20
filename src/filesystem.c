#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../include/bool.h"
#include "../include/defaults.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

int get_dte_len(const dirT_entry *dte) {
	return (sizeof(bool) * 2) + (sizeof(unsigned short) * 2) + dte->nameLen +
		   dte->parentNameLen + (sizeof(size_t) * 2);
}

FILE *fs	  = NULL;
fs_table dirT = {.isFat = false, .size = 0, .u.dirTEntries = NULL};
fs_table faT  = {.isFat = true, .size = 0, .u.fatEntries = NULL};

/**
 * @brief serialises all the entries of a table to a buffer. Grows the buffer if
 * necessary.
 *
 * @return size of the buffer after filling it up
 */
size_t dump_entries_to_buf(fs_table *t) {
	size_t idx = 0;
	size_t cap = 0;

	for (size_t i = 0; i < dirT.size; i++)
		cap += get_dte_len(&dirT.u.dirTEntries[i]);

	char *buf = malloc(cap);
	if (buf == NULL) {
		perror("malloc() in dump_entries_to_buf()");
		return 0;
	}

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

	/* CHECK: Do I need any other information? */
	for (size_t i = 0; i < faT->size; i++)
		faT->u.fatEntries[i] = (fat_entry){
			.used = 0, .nextBlockNum = i + 1, .physicalBlockNum = i};

	faT->u.fatEntries[faT->size].nextBlockNum = SIZE_MAX;
}

/**
 * @details creates a basic filesystem file as per the settings defined in the
 * parameters, generates a preliminary, empty table, writes it to the filesystem
 * file, and populates the directory and FAT tables
 */
bool create_fs(const fs_settings *fss) {
	char *buf			   = NULL;
	const size_t numBlocks = (fss->fsSize * 1024 * 1024) / fss->blockSize;

	/* open file for writing */
	if ((fs = fopen(fss->fsName, "w+")) == NULL) {
		perror("fopen() in create_fs()");
		return false;
	}

	/* create relevant tables in memory */
	init_new_dir_t(fss->numEntries, &dirT);
	if (dirT.u.dirTEntries == NULL)
		goto fclose;

	/* CHECK: how can we figure this out? */
	int numMetadataBlocks = 4;

	init_new_fat(numBlocks, numMetadataBlocks, &faT);
	if (faT.u.fatEntries == NULL) {
		free(dirT.u.dirTEntries);
		goto fclose;
	}

	/* write garbage blocks to disk */
	buf = calloc(fss->blockSize, sizeof(char));
	if (buf == NULL) {
		perror("calloc() in create_fs()");
		free(dirT.u.dirTEntries);
		free(faT.u.fatEntries);
		goto fclose;
	}

	for (size_t i = 0; i < numBlocks; i++) {
		if (write_block(i, fss->blockSize, buf) != 0) {
			fprintf(stderr, "create_fs(): failed at block #%ld\n", i);
			free(dirT.u.dirTEntries);
			free(faT.u.fatEntries);
			free(buf);
			goto fclose;
		}
	}

	return true;

fclose:
	if (fclose(fs) == EOF)
		perror("fclose() in create_fs()");
	return false;
}

int main(/* int argc, char** argv*/) {
	int ret = 0;

	/* user settings */
	fs_settings userFsSettings = load_config();

	/* init filesystem file */
	if (!init_fs(&userFsSettings))
		return 1;

	/* TODO: menu loop w/ ncurses */

	/* cleanup: */
	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	return ret;
}

fs_settings load_config() {
	fs_settings cfg = DEFAULT_CFG;

	/* TODO: read config file to load parameters and deserialise into struct */
	return cfg;
}

/**
 * @brief writes a directory table entry to a buffer. It is the caller's
 * responsibility to ensure that the buffer is large enough to hold the entry
 *
 * @param i idx of the buffer
 */
void write_entry_to_buf(const dirT_entry *e, char *b, size_t *i) {
	memcpy(b + *i, &e->valid, sizeof(bool));
	*i += sizeof(bool);
	memcpy(b + *i, &e->isDir, sizeof(bool));
	*i += sizeof(bool);
	memcpy(b + *i, &e->nameLen, sizeof(unsigned short));
	*i += sizeof(unsigned short);
	memcpy(b + *i, &e->parentNameLen, sizeof(unsigned short));
	*i += sizeof(unsigned short);
	memcpy(b + *i, e->name, e->nameLen);
	*i += e->nameLen;
	memcpy(b + *i, e->parentDir, e->parentNameLen);
	*i += e->parentNameLen;
	memcpy(b + *i, &e->size, sizeof(size_t));
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
			return create_fs(fss);
		} else {
			perror("fopen() in init_fs()");
			return false;
		}
	} else {
		/* TODO: load faT and dirT from disk */
	}

	return true;
}
