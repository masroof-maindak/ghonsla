#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../include/ghonsla.h"
#include "../include/utils.h"

FILE *fs = NULL;

int main(int argc, char **argv) {
	int ret		 = 0;
	fs_table dt	 = {.size = 0, .dirs = NULL};
	fs_table fat = {.size = 0, .blocks = NULL};
	struct fs_settings fss;

	if (!init(&fss, argc, argv, &dt, &fat))
		return 1;

	/* TODO: ncurses menu loop */

	tests(&fss, &dt, &fat);

	/* cleanup: */
	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	free(dt.dirs);
	free(fat.blocks);
	return ret;
}

/**
 * @details opens the filesystem file if it exists, or creates a new one if not
 */
bool init(struct fs_settings *fss, int argc, char **argv, fs_table *dt,
		  fs_table *fat) {
	switch (open_fs(fss, argc, dt, fat)) {
	case 0: /* failed to open file */
		return false;
	case 1: /* file not present */
		if (!load_config(fss, argc, argv) || !init_new_fs(fss, dt, fat))
			return false;
		break;
	case 2: /* file present and loaded */
		break;
	}

	return true;
}

void tests(struct fs_settings *fss, fs_table *dt, fs_table *fat) {
	char *firstDir	= copy_string("firstDir");
	char *f1name	= copy_string("f1");
	char *f2name	= copy_string("f2");
	char *f3name	= copy_string("f3");
	char *rename	= copy_string("f2_renamed");
	char *secondDir = copy_string("secondDir");
	char *f4name	= copy_string("f4");
	size_t idx		= ROOT_IDX;

	create_dir_entry(firstDir, ROOT_IDX, true, dt);
	create_dir_entry(f1name, ROOT_IDX, false, dt);

	idx = get_index_of_dir_entry(f1name, ROOT_IDX, dt);
	remove_dir_entry(idx, dt, fat);

	create_dir_entry(f2name, 1, false, dt);
	create_dir_entry(f3name, 1, false, dt);

	idx = get_index_of_dir_entry(f2name, 1, dt);
	rename_dir_entry(rename, idx, dt);

	printf("firstDir\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	print_directory_contents(idx, dt);
	create_dir_entry(secondDir, 1, true, dt);
	puts("");

	idx = get_index_of_dir_entry(secondDir, 1, dt);
	create_dir_entry(f4name, idx, false, dt);
	printf("secondDir\n");
	print_directory_contents(idx, dt);
	puts("");

	printf("firstDir\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	print_directory_contents(idx, dt);
	puts("");

	printf("/:\n");
	print_directory_contents(ROOT_IDX, dt);
	puts("");

	quick_format_fs(fss, dt, fat);

	printf("/:\n");
	print_directory_contents(ROOT_IDX, dt);

	/* free(firstDir); */
	/* free(f3name); */
	/* free(rename); */
	/* free(secondDir); */
	/* free(f4name); */
}
