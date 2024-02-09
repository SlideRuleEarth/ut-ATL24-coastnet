#!/usr/bin/bash

# Bash strict mode, without pipefail
set -eu
IFS=$'\n\t'

find ./data/local/3DGL/*.csv | head -10 | build/debug/train_coastnet \
    --verbose \
    --epochs=3 \
    --max-samples-per-class=5000 \
    --network-filename=coastnet.pt
