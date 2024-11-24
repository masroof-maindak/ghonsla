#!/usr/bin/env bash

bp="deserialise_metadata"
bin="./ghonsla"

# rm disk.fs
make
gdb --quiet -ex "break ${bp}" -ex 'run' "$bin"
