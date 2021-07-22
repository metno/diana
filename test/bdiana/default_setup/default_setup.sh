#!/bin/bash
set -e

. bdiana.sh

TEST=default_setup
cd "$TEST"

PNG=`get_filename "$TEST.input"`
rm -f "$TEST.out" "$PNG"
run_bdiana -i "$TEST.input" "$@" > "$TEST.out" 2>&1 # no setup file option

# PNG should be present
test -s "$PNG"
