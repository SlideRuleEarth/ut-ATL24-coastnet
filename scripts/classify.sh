#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

mkdir -p predictions
for n in $(seq 0 4)
do
    parallel --lb --dry-run \
        "build/release/classify --verbose --num-classes=7 --network-filename=coastnet_network-${n}.pt --results-filename=predictions/{/.}_results-${n}.txt < {} > predictions/{/.}_classified_${n}.csv" \
        :::: coastnet_test_files-${n}.txt
done
