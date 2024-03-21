#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/score_coastnet \
        --verbose \
        --class=$2 \
        < {} > {.}_results.txt"
