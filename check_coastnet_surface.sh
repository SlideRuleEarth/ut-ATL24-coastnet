#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/blunder_detection \
        --verbose \
        < {} > $2/{/.}_checked_surface.csv"
