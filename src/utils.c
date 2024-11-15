#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"

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

