#!/usr/bin/bash

# Bash strict mode
set -eu
IFS=$'\n\t'

input=$(ls -1 ${1})
suffix=${2}

build/debug/score --verbose \
    --ignore-class=41 --class=40 \
    --csv-filename=micro_scores_no_surface${suffix}.csv \
    ${input} \
    > micro_scores_no_surface${suffix}.txt

build/debug/score --verbose \
    --csv-filename=micro_scores_all${suffix}.csv \
    ${input} \
    > micro_scores_all${suffix}.txt
