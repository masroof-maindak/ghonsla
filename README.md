# Ghonsla

![dinosaur-eggs](.github/assets/dinosaur-eggs.jpg)

Ghonsla is a virtual, persistent, FAT-like filesystem that advertises directory management, partition creation/formatting and file operation APIs, alongside a frankly repulsive (but functional) ncurses-based TUI for directory navigation.

## Installation

```bash
git clone --recursive https://github.com/masroof-maindak/ghonsla.git
make
./ghonsla [-m size-in-MBs] [-n entry-count]  [-s block-size] [-b file-max-block-count]
```

## Usage

| Key        | Action                     |
| :--------- | :------------------------- |
| `KEY_DOWN` | Scroll down                |
| `j`        | Scroll down                |
| `KEY_UP`   | Scroll up                  |
| `k`        | Scroll up                  |
| `Enter`    | Step into child directory  |
| `l`        | Step into child directory  |
| `h`        | Go up to parent directory  |
| `t`        | Create file (touch)        |
| `m`        | Create directory (mkdir)   |
| `r`        | Remove file or directory   |
| `q`        | Quit the application

## TODO

- [ ] Encryption on-disk
- [ ] Optimise disk writes (buffered I/O)?
- [ ] Multi-partition support
