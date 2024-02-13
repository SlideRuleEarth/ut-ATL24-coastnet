#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

mkdir -p predictions

find ./data/local/3DGL/*.csv | \
parallel --lb --dry-run \
    "build/debug/classify_coastnet_surface \
    --verbose \
    --aspect-ratio=1 \
    --network-filename=coastnet-surface.pt \
    --results-filename=predictions/{/.}_results_ws.txt < {} > predictions/{/.}_classified_ws.csv"
