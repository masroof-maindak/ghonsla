#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

char *copy_string(const char *str);
char *double_if_Of(char *buf, size_t idx, size_t add, size_t *size);

int read_block(size_t blockNo, size_t blockSize, char *buf);
int write_block(size_t blockNo, size_t blockSize, char *buf);

#endif // UTILS_H
