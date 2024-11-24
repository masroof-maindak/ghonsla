#ifndef GHONSLA_H
#define GHONSLA_H

#include "filesystem.h"

#define ROOT_IDX 0

void tests_deserialise(fs_table *const dt);
void tests_generate(struct fs_settings *const fss, fs_table *const dt,
					fs_table *const fat);
bool init_fs(struct fs_settings *fss, int argc, char **argv, fs_table *const dt,
			 fs_table *const fat);

#endif // GHONSLA_H
