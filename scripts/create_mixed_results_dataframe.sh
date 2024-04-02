#!/usr/bin/bash

# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

printf "Dataset,cls,Acc,F1,BA,F1_r0,Wght_F1,Wght_F1_r0\n"

for fn in $1
do
    # Strip off dir and extension
    bn=$(basename ${fn})

    # ATL03 granule ID is 41 bytes, first 6 bytes are 'ATL_03'
    id=${bn:6:35}

    # Get Acc, F1, BA, cal_F1
    s1=$(grep "^${2}" ${fn} | tr '\t' ',' | cut -f 1,2,3,4,5 -d ',')

    # Get weighted F1
    s2=$(grep "weighted_F1" ${fn} | tr -d ' ' | cut -f 2 -d '=')

    # Get weighted cal_F1
    s3=$(grep "weighted_cal_F1" ${fn} | tr -d ' ' | cut -f 2 -d '=')

    printf "%s,%s,%s,%s\n" ${id} ${s1} ${s2} ${s3}
done
