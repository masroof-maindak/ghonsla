#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../include/bool.h"
#include "../include/defaults.h"
#include "../include/filesystem.h"
#include "../include/utils.h"

const struct fs_settings defaultCfg = {.fsName	   = FS_NAME,
									   .fsSize	   = FS_SIZE,
									   .numEntries = NUM_ENTRIES,
									   .blockSize  = BLOCK_SIZE,
									   .fBlocks	   = FILE_BLOCKS,
									   .fNameLen   = FILE_NAME_LEN};
const struct table_info emptyTable	= {.table = NULL, .idx = 0, .size = 0};
const struct dir_entry rootEntry	= {.valid		  = true,
									   .isDir		  = true,
									   .nameLen		  = 6,
									   .parentNameLen = 0,
									   .name		  = "_ROOT\0",
									   .parentDir	  = "\0",
									   .size		  = 0,
									   .firstBlockNo  = -1};
const struct dir_entry garbageEntry = {.valid		  = false,
									   .isDir		  = false,
									   .nameLen		  = 8,
									   .parentNameLen = 6,
									   .name		  = "$arbage\0",
									   .parentDir	  = "_ROOT\0",
									   .size		  = 0,
									   .firstBlockNo  = -1};

int get_dts_len(const struct dir_entry *dte) {
	return (sizeof(bool) * 2) + (sizeof(ushort) * 2) + dte->nameLen +
		   dte->parentNameLen + (sizeof(size_t) * 2);
}

#define ROOT_ENTRY_LEN	  get_dts_len(&rootEntry)
#define GARBAGE_ENTRY_LEN get_dts_len(&garbageEntry)

FILE *fs			   = NULL;
struct table_info dirT = emptyTable;
struct table_info fat  = emptyTable;

/**
 * @brief creates a directory table in memory, populates it with the root entry
 * and garbage entries
 */
struct table_info create_dir_table(int entryCount) {
	struct table_info dirT = emptyTable;
	dirT.size			   = entryCount * 64;

	dirT.table = malloc(dirT.size); /* each entry ~= 64 bytes */
	if (dirT.table == NULL) {
		perror("malloc() in create_fs()");
		return dirT;
	}

	if (double_if_Of(dirT.table, dirT.idx, ROOT_ENTRY_LEN, &dirT.size) == NULL)
		return emptyTable;

	const int garbageEntryLen = GARBAGE_ENTRY_LEN;

	write_entry_to_buf(&rootEntry, dirT.table, &dirT.idx);
	for (int i = 1; i < entryCount; i++) {
		if (double_if_Of(dirT.table, dirT.idx, garbageEntryLen, &dirT.size))
			return emptyTable;
		write_entry_to_buf(&garbageEntry, dirT.table, &dirT.idx);
	}

	return dirT;
}

struct table_info create_fat() {
	struct table_info fat = emptyTable;

	/* TODO: create FAT */

	return fat;
}

/**
 * @details creates a basic filesystem file as per the settings defined in the
 * parameters, generates a preliminary, empty table, writes it to the filesystem
 * file, and populates the directory and FAT tables
 */
bool create_fs(const struct fs_settings *fss) {
	/* open file for writing */
	if ((fs = fopen(fss->fsName, "w+")) == NULL) {
		perror("fopen() in create_fs()");
		return false;
	}

	/* create relevant tables in memory */
	dirT = create_dir_table(fss->numEntries);
	if (dirT.table == NULL) {
		if (fclose(fs) == EOF)
			perror("fclose() #1 in create_fs()");
		return false;
	}

	fat = create_fat();
	if (fat.table == NULL) {
		if (fclose(fs) == EOF)
			perror("fclose() #2 in create_fs()");
		return false;
	}

	/* TODO: while tableSize > 0, write new blocks to disk... */

	return true;
}

int main(/* int argc, char** argv*/) {
	int ret = 0;

	/* user settings */
	struct fs_settings userFsSettings = load_config();

	/* init filesystem file */
	if (!init_fs(&userFsSettings))
		goto cleanup;

	/* TODO: menu loop w/ ncurses */

cleanup:
	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	return ret;
}

struct fs_settings load_config() {
	/* TODO: read config file to load parameters and deserialise into struct */
	return defaultCfg;
}

/**
 * @brief writes a directory table entry to a buffer. It is the caller's
 * responsibility to ensure that the buffer is large enough to hold the entry
 */
void write_entry_to_buf(const struct dir_entry *e, char *b, size_t *s) {
	memcpy(b + *s, &e->valid, sizeof(bool));
	*s += sizeof(bool);
	memcpy(b + *s, &e->isDir, sizeof(bool));
	*s += sizeof(bool);
	memcpy(b + *s, &e->nameLen, sizeof(ushort));
	*s += sizeof(ushort);
	memcpy(b + *s, &e->parentNameLen, sizeof(ushort));
	*s += sizeof(ushort);
	memcpy(b + *s, e->name, e->nameLen);
	*s += e->nameLen;
	memcpy(b + *s, e->parentDir, e->parentNameLen);
	*s += e->parentNameLen;
	memcpy(b + *s, &e->size, sizeof(size_t));
	*s += sizeof(size_t);
	memcpy(b + *s, &e->firstBlockNo, sizeof(size_t));
	*s += sizeof(size_t);
}

/**
 * @brief creates or opens an existing filesystem file
 */
bool init_fs(const struct fs_settings *fss) {
	if ((fs = fopen(FS_NAME, "r+")) == NULL) {
		if (errno == ENOENT) {
			if (!create_fs(fss))
				return false;
		} else {
			perror("fopen() in init_fs()");
			return false;
		}
	}
	return true;
}
