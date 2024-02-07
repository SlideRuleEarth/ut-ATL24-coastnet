#!/usr/bin/bash

printf "Bathy F1 = "
cat predictions/*.txt | grep "^40" | cut -f 3 | datamash mean 1
printf "Bathy BA = "
cat predictions/*.txt | grep "^40" | cut -f 4 | datamash mean 1
printf "Average weighted F1 = "
cat predictions/*.txt | grep _F1 | cut -f 2 -d '=' | datamash mean 1
printf "Average weighted BA = "
cat predictions/*.txt | grep _bal_acc | cut -f 2 -d '=' | datamash mean 1
