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
        < {} > $2/{/.}_checked_bathy.csv"

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/score_coastnet \
        --verbose \
        --class=41 \
        < $2/{/.}_checked_bathy.csv > $2/{/.}_results_checked_surface.txt"

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/score_coastnet \
        --verbose \
        --class=40 \
        < $2/{/.}_checked_bathy.csv > $2/{/.}_results_checked_bathy.txt"
