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

x1=$(grep "Average Acc" $1 | cut -f 2 -d '=' | datamash mean 1)
x2=$(grep "Average F1" $1 | cut -f 2 -d '=' | datamash mean 1)
x3=$(grep "Average cal_F1" $1 | cut -f 2 -d '=' | datamash mean 1)
x4=$(grep "Average Weighted F1" $1 | cut -f 2 -d '=' | datamash mean 1)
x5=$(grep "Average Weighted cal_F1" $1 | cut -f 2 -d '=' | datamash mean 1)
x6=$(grep "Average BA" $1 | cut -f 2 -d '=' | datamash mean 1)

printf "avg,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f\n"\
    ${x1} ${x2} ${x3} ${x4} ${x5} ${x6}
