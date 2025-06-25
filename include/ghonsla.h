#ifndef GHONSLA_H
#define GHONSLA_H

#include "filesystem.h"

void tests_deserialise(fs_table *const dt);
void tests_generate(struct fs_settings *const fss, fs_table *const dt,
					fs_table *const fat);
_bool init_fs(struct fs_settings *fss, int argc, char **argv, fs_table *const dt,
			 fs_table *const fat);

#endif // GHONSLA_H
