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

# Cross validate
folds=5

for fold in $(seq 0 $((folds-1)))
do
    echo fold $((fold+1)) of ${folds}

    # Setup directories
    training_dir=${tmpdir}/training_files
    testing_dir=${tmpdir}/testing_files
    predictions_dir=${tmpdir}/predictions
    mkdir -p ${training_dir}
    mkdir -p ${testing_dir}
    mkdir -p ${predictions_dir}

    # Get the commands to copy files into training and testing folders
    python3 ./scripts/get_copy_commands.py \
        --fold=${fold} \
        --folds=${folds} \
        --training_dir=${training_dir} \
        --testing_dir=${testing_dir} \
        --filename_glob="$1" > ${tmpdir}/copy_commands.txt

    # Execute the commands
    bash ${tmpdir}/copy_commands.txt

    # Show directory structure
    tree ${tmpdir}

    # Run the cross-validation for this fold

    # Train
    build=${build} ./scripts/train_coastnet_bathy.sh \
        "${training_dir}/*.csv" \
        ${tmpdir}/coastnet-bathy.pt


    # Predict
    build=${build} ./scripts/classify_coastnet_bathy.sh \
        "${testing_dir}/*.csv" \
        ${tmpdir}/coastnet-bathy.pt \
        ${tmpdir}/predictions

    # Score
    ./scripts/get_coastnet_bathy_scores.sh \
        "${tmpdir}/predictions/*_bathy.txt" > ./bathy_fold${fold}_score.txt

    # Save results
    cat ${tmpdir}/predictions/*_bathy.txt > ./bathy_fold${fold}_results.txt

    # Cleanup
    rm -rf ${training_dir}
    rm -rf ${testing_dir}
    rm -rf ${predictions_dir}
done
