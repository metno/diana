#!/bin/bash
set -ex

. bdiana.sh

TEST=field_fail_on_missing

# bdiana should fail
run_bdiana_test "$TEST" && exit 1

# PNG should be absent
test ! -r `get_filename "$TEST.input"`
