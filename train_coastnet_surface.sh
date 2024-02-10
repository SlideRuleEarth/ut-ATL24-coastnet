#!/usr/bin/bash

# Bash strict mode, without pipefail
set -eu
IFS=$'\n\t'

find ./data/local/3DGL/*.csv | head -20 | build/debug/train_coastnet_surface \
    --verbose \
    --epochs=20 \
    --total-samples=20000 \
    --network-filename=coastnet-surface.pt
