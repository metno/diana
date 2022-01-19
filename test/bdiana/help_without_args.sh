#!/bin/bash
set -e

. bdiana.sh

# bdiana should print help when called without arguments
if run_bdiana | grep -q 'DIANA batch' ; then
    echo "OK"
else
    echo "FAIL help message should be shown"
    exit 1
fi
