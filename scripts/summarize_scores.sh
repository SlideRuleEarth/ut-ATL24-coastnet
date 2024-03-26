#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Average Acc = "
cat $1 | grep "^$2" | cut -f 2 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average F1 = "
cat $1 | grep "^$2" | cut -f 3 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average BA = "
cat $1 | grep "^$2" | cut -f 4 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average cal_F1 = "
cat $1 | grep "^$2" | cut -f 5 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Weighted F1 = "
cat $1 | grep "weighted_F1" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
printf "Average Weighted cal_F1 = "
cat $1 | grep "weighted_cal_F1" | cut -f 3 -d ' ' | grep -v nan | datamash --format="%0.3f" mean 1
