#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Average Bathy Acc = "
cat $1 | grep "^40" | cut -f 2 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Bathy F1 = "
cat $1 | grep "^40" | cut -f 3 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Bathy BA = "
cat $1 | grep "^40" | cut -f 4 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Bathy Acc = "
cat $1 | grep "weighted_acc" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Bathy F1 = "
cat $1 | grep "weighted_F1" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Bathy BA = "
cat $1 | grep "weighted_bal_acc" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
