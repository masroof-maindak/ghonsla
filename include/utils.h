#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

#define RESET		 "\x1b[0m"
#define RED			 "\x1b[31m"
#define GREEN		 "\x1b[32m"
#define YELLOW		 "\x1b[33m"
#define BLUE		 "\x1b[34m"
#define MAGENTA		 "\x1b[35m"
#define CYAN		 "\x1b[36m"
#define WHITE		 "\x1b[37m"
#define BOLD_RED	 "\x1b[1;31m"
#define BOLD_GREEN	 "\x1b[1;32m"
#define BOLD_YELLOW	 "\x1b[1;33m"
#define BOLD_BLUE	 "\x1b[1;34m"
#define BOLD_MAGENTA "\x1b[1;35m"
#define BOLD_CYAN	 "\x1b[1;36m"
#define BOLD_WHITE	 "\x1b[1;37m"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

char *copy_string(const char *str);
char *double_if_Of(char *buf, size_t idx, size_t add, size_t *size);
void parse_and_set_ul(unsigned long *dst, char *src);

int read_block(size_t blockNo, size_t blockSize, char *buf);
int write_block(size_t blockNo, size_t blockSize, const char *buf);

#endif // UTILS_H
