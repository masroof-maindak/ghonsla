#ifndef GHONSLA_H
#define GHONSLA_H

#include "filesystem.h"

void tests_deserialise(dir_table *const dt);
void tests_generate(struct fs_settings *const fss, dir_table *const dt);
bool init_fs(struct fs_settings *fss, int argc, char **argv,
			 dir_table *const dt);

#endif // GHONSLA_H
