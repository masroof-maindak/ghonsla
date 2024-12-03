# Ghonsla

### Usage

```bash
git clone --recursive https://github.com/masroof-maindak/ghonsla.git
make
./ghonsla [-m size-in-MBs] [-n entry-count]  [-s block-size] [-b file-max-block-count]
```

### TODO

- [x] read/write\_block
- [x] Global directory table/FAT primitives
- [x] Directories
	- [x] Create
    - [x] Rename
	- [x] List Contents
	- [x] Delete
- [x] Partition
	- [x] Create
        - [x] Parametrisation
        - [x] User input
	- [x] Format
- [x] Files
	- [x] Create
	- [x] Delete
	- [x] Rename
	- [x] Write
	- [x] Truncate
	- [x] Read
    - [ ] Insert?
- [x] Directory table/FAT Persistence
    - [x] Serialise
    - [x] Deserialise
- [x] ~~Termbox2~~Ncurses TUI
    - [x] Print filesystem settings on top/bottom row?
- [ ] Encryption on-disk
- [ ] Optimise disk writes (buffered I/O)?
- [ ] Multi-partition support?
