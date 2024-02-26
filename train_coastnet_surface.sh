#!/usr/bin/bash

# Bash strict mode, without pipefail
set -eu
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

find $1 | \
    ./build/${build}/train_coastnet \
    --verbose \
    --class=41 \
    --epochs=50 \
    --total-samples-per-class=1000000 \
    --batch-size=512 \
    --aspect-ratio=1 \
    --network-filename=$2
