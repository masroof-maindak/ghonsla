CFLAGS = -Wall -Wextra -pedantic
LDFLAGS = -lcurses -lmenu
RELEASE_FLAGS = -march=native -O3
DEBUG_FLAGS = -g3 -O0

SRCDIR = src
LIBDIR = lib
SRCS = $(wildcard ${SRCDIR}/*.c)
TARGET = ghonsla

default: debug

$(TARGET): $(LIBARCS)
	gcc $(SRCS) $(CFLAGS) $(RELEASE_FLAGS) $(LDFLAGS) -o $(TARGET)

debug: $(TARGET)
	gcc $(SRCS) $(CFLAGS) $(DEBUG_FLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f *.o ghonsla disk.fs
	$(MAKE) -C ${TBDIR} clean

.PHONY: clean debug
