#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/bool.h"
#include "../include/defaults.h"

struct fs_settings {
	char *fsName;
	size_t fsSize;
	size_t numEntries;
	size_t blockSize;
	size_t fBlocks;
	size_t fNameLen;
} defaultFsSettings = {FS_NAME,	   FS_SIZE,		NUM_ENTRIES,
					   BLOCK_SIZE, FILE_BLOCKS, FILE_NAME_LEN};

FILE *fs = NULL;

/**
 * @brief creates a basic filesystem file as per the settings defined in the
 * parameter and generates a prelimiary, empty table
 */
bool create_fs(const struct fs_settings *fss) {
	fs = fopen(fss->fsName, "w+");
	if (fs == NULL) {
		perror("fopen() in create_fs()");
		return false;
	}

	/* TODO: make basic directory table & garbage 64 MB file */

	return true;
}

/**
 * @brief creates or opens an existing filesystem file
 */
bool init_fs(const struct fs_settings *fss) {
	fs = fopen(FS_NAME, "r+");

	if (fs == NULL) {
		switch (errno) {
		case ENOENT:
			if (!create_fs(fss))
				return false;
			break;
		default:
			perror("fopen() in init_fs");
			return false;
		}
	}

	return true;
}

struct fs_settings load_config() {
	/* TODO: read config file to load parameters and deserialise into struct */
	return defaultFsSettings;
}

int main(/* int argc, char** argv*/) {
	int ret = 0;

	/* user settings */
	struct fs_settings userFsSettings = load_config();

	/* global file */
	if (!init_fs(&userFsSettings))
		goto cleanup;

	/* TODO: menu loop w/ ncurses */

cleanup:
	if (fclose(fs) == EOF)
		perror("fclose():");
	return ret;
}
