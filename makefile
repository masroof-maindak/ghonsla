CFLAGS = -Wall -Wextra -pedantic
# LDFLAGS = -lcurses -lmenu
DEBUG_FLAGS = -g3
RELEASE_FLAGS = -march=native -O3

SRCDIR = src
SRCS = $(wildcard ${SRCDIR}/*.c)
TARGET = ghonsla

default: debug

$(TARGET):
	gcc $(SRCS) $(CFLAGS) $(RELEASE_FLAGS) $(LDFLAGS) -o $(TARGET)

debug:
	gcc $(SRCS) $(CFLAGS) $(DEBUG_FLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f *.o ghonsla
