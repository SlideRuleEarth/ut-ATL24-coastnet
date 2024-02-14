#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Surface F1 = "
cat $1 | grep "^41" | cut -f 3 | datamash mean 1
printf "Surface BA = "
cat $1 | grep "^41" | cut -f 4 | datamash mean 1
