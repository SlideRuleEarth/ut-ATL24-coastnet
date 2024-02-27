#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

find $1 | \
    parallel --verbose --lb --jobs=15 \
        "python ./preprocess.py {} $2/{/}"
