#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

mkdir -p predictions

find ./data/local/3DGL/*.csv | \
parallel --lb --dry-run \
    "build/debug/classify_coastnet \
    --verbose \
    --network-filename=coastnet.pt \
    --results-filename=predictions/{/.}_results.txt < {} > predictions/{/.}_classified.csv"
