#!/usr/bin/env bash

bp="filesystem.c:151"
bin="./ghonsla"

rm disk.fs
make
gdb --quiet -ex "break ${bp}" -ex 'run' "$bin"
