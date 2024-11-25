#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "../include/defaults.h"
#include "../include/ghonsla.h"
#include "../include/utils.h"
#include "../lib/termbox2/termbox2.h"

FILE *fs = NULL;

int main(int argc, char **argv) {
	int ret				   = 0;
	fs_table dt			   = {.size = 0, .dirs = NULL};
	fs_table fat		   = {.size = 0, .blocks = NULL};
	struct fs_settings fss = DEFAULT_CFG;

	if (!init_fs(&fss, argc, argv, &dt, &fat))
		return 1;

	/* TODO: tb2 menu loop */
	struct tb_event ev;
	tb_init();
	tb_poll_event(&ev);
	tb_shutdown();
	/* ------------------- */

	/* cleanup: */
	serialise_metadata(&fss, &dt, &fat);
	format_fs(&fss, &dt, &fat);

	printf("/:\n");
	print_directory_contents(ROOT_IDX, &dt);

	if (fclose(fs) == EOF)
		perror("fclose() in main()");
	free(dt.dirs);
	free(fat.blocks);
	return ret;
}

/**
 * @details opens the filesystem file if it exists, or creates a new one if not
 */
bool init_fs(struct fs_settings *fss, int argc, char **argv, fs_table *const dt,
			 fs_table *const fat) {

	/* couldn't open */
	if ((fs = fopen(FS_NAME, "r+")) == NULL && errno != ENOENT) {
		perror("fopen() in init_fs()");
		return false;
	}

	/* generate */
	if (errno == ENOENT) {
		if (!parse_config_args(fss, argc, argv) || !init_new_fs(fss, dt, fat))
			return false;
		/* tests_generate(fss, dt, fat); */
		return true;
	}

	/* open and reload */
	if (argc > 1)
		printf("Disk file found, ignoring args\n");

	return deserialise_metadata(fss, dt, fat);
	/* if (!deserialise_metadata(fss, dt, fat)) */
	/* 	return false; */

	/* tests_deserialise(dt); */

	/* return true; */
}

void tests_deserialise(fs_table *const dt) {
	char *firstDir	= "firstDir";
	char *secondDir = "secondDir";
	size_t idx		= ROOT_IDX;

	printf("firstDir:\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	print_directory_contents(idx, dt);
	puts("");

	idx = get_index_of_dir_entry(secondDir, 1, dt);
	printf("secondDir:\n");
	print_directory_contents(idx, dt);
	puts("");

	printf("/:\n");
	print_directory_contents(ROOT_IDX, dt);
	puts("");
}

void tests_generate(struct fs_settings *const fss, fs_table *const dt,
					fs_table *const fat) {
	char *firstDir	= copy_string("firstDir");
	char *f1name	= copy_string("f1");
	char *f2name	= copy_string("f2");
	char *f3name	= copy_string("f3");
	char *rename	= copy_string("f2_renamed");
	char *secondDir = copy_string("secondDir");
	char *f4name	= copy_string("f4");
	/* these get freed @ format time or by rename/remove operations */

	create_dir_entry(firstDir, ROOT_IDX, true, dt);
	create_dir_entry(f1name, ROOT_IDX, false, dt);

	size_t idx = get_index_of_dir_entry(f1name, ROOT_IDX, dt);
	remove_dir_entry(idx, dt, fat);
	f1name = NULL;

	create_dir_entry(f2name, 1, false, dt);
	create_dir_entry(f3name, 1, false, dt);

	idx = get_index_of_dir_entry(f2name, 1, dt);
	rename_dir_entry(rename, idx, dt);
	f2name = NULL;

	printf("firstDir:\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	print_directory_contents(idx, dt);
	create_dir_entry(secondDir, 1, true, dt);
	puts("");

	idx = get_index_of_dir_entry(secondDir, 1, dt);
	create_dir_entry(f4name, idx, false, dt);
	printf("secondDir:\n");
	print_directory_contents(idx, dt);
	puts("");

	printf("firstDir:\n");
	idx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	print_directory_contents(idx, dt);
	puts("");

	printf("/:\n");
	print_directory_contents(ROOT_IDX, dt);
	puts("");

	size_t pidx = get_index_of_dir_entry(firstDir, ROOT_IDX, dt);
	idx			= get_index_of_dir_entry(f3name, pidx, dt);
	char s1[]	= "I am writing some text!";
	char s2[]	= "I am writing some more text!";
	char s3[]	= "overwritten";
	int w1 = 1020, w2 = 1024;
	char str1[w1], str2[w2];
	memset(str1, '_', w1);
	int x;
	if ((x = write_to_file(idx, str1, w1, fss, 0, dt, fat)) < 0)
		printf("write #1 %d\n", x);
	if ((x = write_to_file(idx, s1, strlen(s1), fss, 0, dt, fat)) < 0)
		printf("write #2 %d\n", x);
	if ((x = append_to_file(idx, s2, strlen(s2), fss, dt, fat)) < 0)
		printf("write #3 %d\n", x);
	if ((x = write_to_file(idx, s3, strlen(s3), fss, 5, dt, fat)) < 0)
		printf("write #4 %d\n", x);
	if ((x = read_file_at(idx, str2, w2, fss, 0, dt, fat)) < 0)
		printf("read #1 %d\n", x);
	if ((x = read_file_at(idx, str2, w2, fss, 3, dt, fat)) < 0)
		printf("read #2 %d\n", x);
}
