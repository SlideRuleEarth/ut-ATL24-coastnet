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

./scripts/create_xval_results_dataframe.sh \
    "results/manual-surface_fold?_summary.txt" \
    > ${tmpdir}/df.csv
python ./scripts/plot_fold_summaries.py \
    ${tmpdir}/df.csv surface_fold_summary.png
python ./scripts/create_fold_summary_table.py \
    ${tmpdir}/df.csv surface_fold_summary_table.png

./scripts/create_xval_results_dataframe.sh \
    "results/manual-bathy_fold?_summary.txt" \
    > ${tmpdir}/df.csv
python ./scripts/plot_fold_summaries.py \
    ${tmpdir}/df.csv bathy_fold_summary.png
python ./scripts/create_fold_summary_table.py \
    ${tmpdir}/df.csv bathy_fold_summary_table.png

eog *_fold_summary.png *_fold_summary_table.png
