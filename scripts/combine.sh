#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Use debug build unless overridden from cmdline
: ${build:=debug}

echo build = ${build}

# Trap on exit and error signals
trap cleanup SIGINT SIGTERM ERR EXIT

# Create a temporary directory
tmpdir=$(mktemp --tmpdir --directory tmp.XXXXXXXX)

function cleanup {
    echo "Cleaning up..."
    trap - SIGINT SIGTERM ERR EXIT
    rm -rf ${tmpdir}
}

ID=$1
find ./input/${ID}/*.csv  > ${tmpdir}/fns.txt
parallel basename {} .csv :::: ${tmpdir}/fns.txt > ${tmpdir}/basenames.txt
parallel \
    "./build/${build}/combine_predictions \
    ./predictions/${ID}/{}_classified_surface.csv \
    ./predictions/${ID}/{}_classified_bathy.csv \
    > ./predictions/${ID}/{}_classified_combined.csv" \
    :::: ${tmpdir}/basenames.txt
