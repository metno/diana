#!/bin/bash
set -e

. bdiana.sh

TEST=obs_fail_on_error

# bdiana should fail
run_bdiana_test "$TEST" && exit 1

# PNG should be absent
test ! -r `get_filename "$TEST.input"`
