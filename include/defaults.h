#ifndef DEFAULTS_H
#define DEFAULTS_H

#define FS_NAME		  "disk.fs" /* filesystem name on disk */
#define FS_SIZE		  64		/* filesystem size in megabytes */
#define NUM_ENTRIES	  128		/* number of file entries in the dir table */
#define BLOCK_SIZE	  1024		/* number of bytes given to one block */
#define FILE_BLOCKS	  128		/* number of blocks given to a file */
#define FILE_NAME_LEN 64		/* max length of a file's name */

#define DIR_TABLE_ROOT_ENTRY                                                   \
	(dirT_entry){.valid			= true,                                        \
				 .isDir			= true,                                        \
				 .nameLen		= 6,                                           \
				 .parentNameLen = 0,                                           \
				 .name			= "_ROOT\0",                                   \
				 .parentDir		= "\0",                                        \
				 .size			= 0,                                           \
				 .firstBlockNum = SIZE_MAX};

#define DIR_TABLE_GARBAGE_ENTRY                                                \
	(dirT_entry){.valid			= false,                                       \
				 .isDir			= false,                                       \
				 .nameLen		= 8,                                           \
				 .parentNameLen = 6,                                           \
				 .name			= "$arbage\0",                                 \
				 .parentDir		= "_ROOT\0",                                   \
				 .size			= 0,                                           \
				 .firstBlockNum = SIZE_MAX};

#define DEFAULT_CFG                                                            \
	(fs_settings){.fsName	  = FS_NAME,                                       \
				  .fsSize	  = FS_SIZE,                                       \
				  .numEntries = NUM_ENTRIES,                                   \
				  .blockSize  = BLOCK_SIZE,                                    \
				  .fBlocks	  = FILE_BLOCKS,                                   \
				  .fNameLen	  = FILE_NAME_LEN};

#endif // DEFAULTS_H
