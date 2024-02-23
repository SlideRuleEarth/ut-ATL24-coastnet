#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Bathy F1 = "
cat $1 | grep "^40" | cut -f 3 | grep -v nan | datamash mean 1
printf "Bathy BA = "
cat $1 | grep "^40" | cut -f 4 | grep -v nan | datamash mean 1
