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
- [ ] Basic menu
- [/] Files
	- [x] Create
	- [x] Delete
	- [x] Rename
	- [ ] Read
	- [ ] Write
        - [/] Append
        - [ ] Insert
	- [/] Truncate
- [x] Directories
	- [x] Create
    - [x] Rename
	- [x] List Contents
	- [x] Delete
- [ ] Directory table/FAT Persistence
    - [/] Serialise
    - [ ] Deserialise
- [x] Partition
	- [x] Create
        - [x] Parametrisation
        - [x] User input
	- [x] Format
- [ ] Encryption on-disk
- [ ] Ncurses/Termbox2 TUI
    - [ ] Print filesystem settings on top/bottom row
- [ ] Optimise disk writes
- [ ] Multi-partition support?
