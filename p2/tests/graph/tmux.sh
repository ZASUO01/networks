#!/bin/sh
set -eu

exe="../../bin/main"
letters="a b c d e f"

i=1
for letter in $letters; do
    tmux split-pane -v "$exe 127.0.1.$i 4 $letter.txt" &
    tmux select-layout even-vertical
    i=$((i + 1))
done
