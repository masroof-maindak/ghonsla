# Ghonsla

![dinosaur-eggs](.github/assets/dinosaur-eggs.jpg)

Ghonsla is a virtual, persistent, FAT-like filesystem that advertises directory management, partition creation/formatting and file operation APIs, alongside a frankly repulsive ncurses-based TUI for directory navigation.

## Usage

```bash
git clone --recursive https://github.com/masroof-maindak/ghonsla.git
make
./ghonsla [-m size-in-MBs] [-n entry-count]  [-s block-size] [-b file-max-block-count]
```

## TODO

- [ ] Encryption on-disk
- [ ] Optimise disk writes (buffered I/O)?
- [ ] Multi-partition support
