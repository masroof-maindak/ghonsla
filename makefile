CFLAGS = -Wall -Wextra -pedantic
LDFLAGS =
RELEASE_FLAGS = -march=native -O3
DEBUG_FLAGS = -g3 -O0

SRCDIR = src
LIBDIR = lib
SRCS = $(wildcard ${SRCDIR}/*.c)
TARGET = ghonsla

TBDIR = $(LIBDIR)/termbox2
TBARC = $(TBDIR)/libtermbox2.a
LIBARCS = $(TBARC)

default: debug

$(TARGET): $(LIBARCS)
	gcc $(SRCS) $(CFLAGS) $(RELEASE_FLAGS) $(LDFLAGS) $(LIBARCS) -o $(TARGET)

debug: $(TARGET)
	gcc $(SRCS) $(CFLAGS) $(DEBUG_FLAGS) $(LDFLAGS) $(LIBARCS) -o $(TARGET)

$(TBARC):
	$(MAKE) -C ${TBDIR} libtermbox2.a

clean:
	rm -f *.o ghonsla disk.fs
	$(MAKE) -C ${TBDIR} clean

.PHONY: clean debug
