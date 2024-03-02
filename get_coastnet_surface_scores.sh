#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Surface Acc = "
cat $1 | grep "^41" | cut -f 2 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Surface F1 = "
cat $1 | grep "^41" | cut -f 3 | grep -v nan | datamash --format="%0.3f" mean 1
printf "Surface BA = "
cat $1 | grep "^41" | cut -f 4 | grep -v nan | datamash --format="%0.3f" mean 1
