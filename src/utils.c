#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

extern FILE *fs;

/**
 * @details if adding `add` bytes to `buf`, (whose maximum capacity is
 * `capacity` and currently has `idx` bytes written), would overflow it, then
 * double `buf`. `capacity` is updated in this case.
 *
 * @return NULL on failure, buffer on success
 */
char *double_if_Of(char *buf, size_t idx, size_t add, size_t *capacity) {
	char *tmp = NULL;

	if (idx + add > *capacity) {
		*capacity *= 2;
		if ((tmp = realloc(buf, *capacity)) == NULL) {
			perror("realloc() in double_if_Of()");
			free(buf);
			return NULL;
		}
	}

	return buf;
}

char *copy_string(const char *str) {
	if (str == NULL) {
		fprintf(stderr, "copy_string(): receieved null input\n");
		return NULL;
	}

	size_t size = strlen(str);
	char *copy	= malloc(size + 1);
	if (copy == NULL) {
		perror("malloc() in copy_string()");
		return NULL;
	}

	memcpy(copy, str, size);
	copy[size] = '\0';
	return copy;
}

int read_block(size_t blockNo, size_t blockSize, char *buf) {
	int ret = 0;
	long save;

	/* save fpos */
	save = ftell(fs);
	if (save == -1) {
		perror("ftell() in read_block()");
		return -1;
	}

	/* goto requested fpos */
	if (fseek(fs, blockSize * blockNo, SEEK_SET) == -1) {
		perror("fseek() #1 in read_block()");
		ret = -2;
		goto cleanup;
	}

	/* read chunk */
	if (fread(buf, 1, blockSize, fs) != blockSize) {
		if (feof(fs)) {
			fprintf(stderr, "fread() in read_block - EOF occurred");
			ret = -3;
			goto cleanup;
		} else if (ferror(fs)) {
			perror("fread() in read_block()");
			ret = -4;
			goto cleanup;
		}
	}

cleanup:
	/* goto original fpos */
	if (fseek(fs, save, SEEK_SET) == -1) {
		perror("fseek() #2 in read_block()");
		return -5;
	}

	return ret;
}

int write_block(size_t blockNo, size_t blockSize, char *buf) {
	int ret = 0;
	long save;

	/* save fpos */
	save = ftell(fs);
	if (save == -1) {
		perror("ftell() in write_block()");
		return -1;
	}

	/* goto requested fpos */
	if (fseek(fs, blockSize * blockNo, SEEK_SET) == -1) {
		perror("fseek() #1 in write_block()");
		ret = -2;
		goto cleanup;
	}

	/* write chunk */
	if (fwrite(buf, 1, blockSize, fs) < blockSize) {
		ret = -3;
		if (ferror(fs))
			perror("fwrite() in write_block()");
	}

cleanup:
	/* reset fpos */
	if (fseek(fs, save, SEEK_SET) == -1) {
		perror("fseek() #2 in write_block()");
		return -4;
	}

	return ret;
}

/**
 * @brief: parses a string to an unsigned long, setting the destination pointer
 *
 * @details if an error occurs, an error message is printed to stderr, so it is
 * optimal to set
 */
void parse_and_set_ul(unsigned long *dst, char *src) {
	char *endptr;

	errno			  = 0;
	unsigned long val = strtoul(src, &endptr, 10);

	if (errno == ERANGE) {
		perror("strtol() in parse_and_set_ul()");
		return;
	}

	if (endptr == src) {
		fprintf(stderr, "%s: No digits found in argument\n", src);
		return;
	}

	*dst = val;
	return;
}
