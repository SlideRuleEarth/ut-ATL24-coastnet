#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

mkdir -p predictions

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "build/${build}/classify_coastnet_surface \
        --verbose \
        --aspect-ratio=1 \
        --network-filename=coastnet-surface.pt \
        --results-filename=predictions/{/.}_results_surface.txt < {} > predictions/{/.}_classified_surface.csv"
