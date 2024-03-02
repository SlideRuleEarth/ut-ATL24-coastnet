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
    --class=40 \
    --epochs=20 \
    --total-samples-per-class=500000 \
    --batch-size=512 \
    --aspect-ratio=4 \
    --network-filename=$2
