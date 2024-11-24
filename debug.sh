#!/usr/bin/env bash

bp="write_to_file"
bin="./ghonsla"

rm disk.fs
make
gdb --quiet -ex "break ${bp}" -ex 'run' "$bin"
