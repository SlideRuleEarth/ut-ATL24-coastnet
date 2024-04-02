#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Trap on exit and error signals
trap cleanup SIGINT SIGTERM ERR EXIT

# Create a temporary directory
tmpdir=$(mktemp --tmpdir --directory tmp.XXXXXXXX)

function cleanup {
    echo "Cleaning up..."
    trap - SIGINT SIGTERM ERR EXIT
    rm -rf ${tmpdir}
}

./scripts/create_mixed_results_dataframe.sh \
    "predictions/mixed/*_surface_results.txt" \
    41 \
    | grep -v nan \
    > ${tmpdir}/df.csv
cat ${tmpdir}/df.csv
python ./scripts/plot_mixed_predictions.py \
    ${tmpdir}/df.csv surface_mixed_summary.png

./scripts/create_mixed_results_dataframe.sh \
    "predictions/mixed/*_bathy_results.txt" \
    40 \
    | grep -v nan \
    > ${tmpdir}/df.csv
cat ${tmpdir}/df.csv
python ./scripts/plot_mixed_predictions.py \
    ${tmpdir}/df.csv bathy_mixed_summary.png

eog *_mixed_summary.png
