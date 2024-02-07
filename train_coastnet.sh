#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

find ./data/local/3DGL/*.csv | build/debug/train_coastnet \
    --verbose \
    --class=41 \
    --max-samples-per-class=1000 \
    --network-filename=coastnet-bathy.pt
