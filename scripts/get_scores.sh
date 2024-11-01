#!/usr/bin/bash

# Bash strict mode
set -eu
IFS=$'\n\t'

input=$(ls -1 ./predictions/*_classified.csv)

build/debug/score --verbose \
    --ignore-class=41 --class=40 \
    --csv-filename=micro_scores_no_surface.csv \
    ${input} \
    > micro_scores_no_surface.txt

build/debug/score --verbose \
    --csv-filename=micro_scores_all.csv \
    ${input} \
    > micro_scores_all.txt
