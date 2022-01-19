#!/bin/bash
set -ex

. bdiana.sh

TEST=field_nofail_missing

# bdiana should not fail
run_bdiana_test "$TEST"

# PNG should be present
test -r `get_filename "$TEST.input"`
