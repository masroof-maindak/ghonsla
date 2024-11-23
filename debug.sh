#!/usr/bin/env bash

bp="quick_format_fs"
bin="./ghonsla"

rm disk.fs
gdb --quiet -ex "break ${bp}" -ex 'run' "$bin"
