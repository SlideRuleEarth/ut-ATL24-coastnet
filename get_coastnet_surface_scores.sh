#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Average Surface Acc = "
cat $1 | grep "^41" | cut -f 2 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Surface F1 = "
cat $1 | grep "^41" | cut -f 3 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Surface BA = "
cat $1 | grep "^41" | cut -f 4 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Surface Acc = "
cat $1 | grep "weighted_acc" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Surface F1 = "
cat $1 | grep "weighted_F1" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
printf "Weighted Surface BA = "
cat $1 | grep "weighted_bal_acc" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
