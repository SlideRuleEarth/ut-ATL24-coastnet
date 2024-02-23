#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/classify_coastnet \
        --verbose \
        --class=40 \
        --aspect-ratio=1 \
        --network-filename=$2 \
        --results-filename=$3/{/.}_results_bathy.txt < {} > $3/{/.}_classified_bathy.csv"
