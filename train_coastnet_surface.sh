#!/usr/bin/bash

# Bash strict mode, without pipefail
set -eu
IFS=$'\n\t'

find ./data/local/3DGL/*.csv |  build/debug/train_coastnet_surface \
    --verbose \
    --epochs=40 \
    --total-samples-per-class=250000 \
    --batch-size=512 \
    --network-filename=coastnet-surface.pt
