#ifndef GHONSLA_H
#define GHONSLA_H

#include "filesystem.h"

#define ROOT_IDX 0

void tests(struct fs_settings *fss, fs_table *dt, fs_table *fat);
bool init(struct fs_settings *fss, int argc, char **argv, fs_table *dt,
		  fs_table *fat);

#endif // GHONSLA_H
