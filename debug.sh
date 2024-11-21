#!/usr/bin/env bash

rm disk.fs
break="rename_dir_entry"
gdb --quiet -ex "break ${break}" -ex 'run' ./ghonsla
