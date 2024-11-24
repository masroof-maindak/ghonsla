# Ghonsla

### Usage

```bash
rm disk.fs # (if present)
make
./ghonsla [-m size-in-MBs] [-n entry-count]  [-s block-size] [-b file-max-block-count]
```

### TODO

- [x] read/write\_block
- [x] Global directory table/FAT primitives
- [ ] ~~Basic menu~~
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
- [ ] Files
	- [x] Create
	- [x] Delete
	- [x] Rename
	- [x] Write
	- [x] Truncate
	- [ ] Read
    - [ ] Insert?
- [ ] Directory table/FAT Persistence
    - [/] Serialise
    - [ ] Deserialise
- [ ] Encryption on-disk
- [ ] Ncurses/Termbox2 TUI
    - [ ] Print filesystem settings on top/bottom row
- [ ] Optimise disk writes (buffered I/O)?
- [ ] Multi-partition support?
