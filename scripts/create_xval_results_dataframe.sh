#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Fold,Acc,F1,BA,F1_r0,Wght_F1,Wght_F1_r0\n"

index=0
for fn in $1
do
    printf "%d," ${index}
    cat ${fn} | cut -f 2 -d '=' | paste -s -d ',' | tr -d ' '
    ((index+=1))
done
