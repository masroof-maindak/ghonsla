#ifndef DEFAULTS_H
#define DEFAULTS_H

#define FS_NAME		"disk.fs" /* filesystem name on disk */
#define FS_SIZE		4		  /* filesystem size in megabytes */
#define NUM_ENTRIES 128		  /* number of file entries in the dir table */
#define BLOCK_SIZE	1024	  /* number of bytes given to one block */
#define FILE_BLOCKS 128		  /* number of blocks given to a file */

#define MAX_INDEX_BLOCKS	  16  /* TODO: make dynamic */
#define MAX_NAME_LEN		  256 /* Maximum length of a file's name */
#define MAX_SIZE_DIR_ENTRY	  /* Largest possible entry in the dir table; we   \
								 might be over-estimating a little because of  \
								 4 bytes' worth of padding in the struct but   \
								 that's fine */                                \
	(sizeof(dir_entry) - sizeof(char *) + MAX_NAME_LEN)

#define DIR_TABLE_ROOT_ENTRY                                                   \
	(dir_entry){.valid	   = true,                                             \
				.isDir	   = true,                                             \
				.nameLen   = 1,                                                \
				.name	   = "/",                                              \
				.size	   = 0,                                                \
				.parentIdx = 0,                                                \
				.idx	   = {0}};

#define DIR_TABLE_GARBAGE_ENTRY                                                \
	(dir_entry){.valid	   = false,                                            \
				.isDir	   = false,                                            \
				.nameLen   = 0,                                                \
				.name	   = "",                                               \
				.size	   = 0,                                                \
				.parentIdx = 0,                                                \
				.idx	   = {0}};

#define DEFAULT_CFG                                                            \
	(struct fs_settings){.size		 = FS_SIZE,                                \
						 .entryCount = NUM_ENTRIES,                            \
						 .blockSize	 = BLOCK_SIZE,                             \
						 .fMaxBlocks = FILE_BLOCKS};

#endif // DEFAULTS_H
