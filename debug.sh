#!/usr/bin/env bash

rm disk.fs
break="quick_format_fs"
gdb --quiet -ex "break ${break}" -ex 'run' ./ghonsla
