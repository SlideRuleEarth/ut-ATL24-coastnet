#!/usr/bin/bash

printf "Surface F1 = "
cat predictions/*_ws.txt | grep "^41" | cut -f 3 | datamash mean 1
printf "Surface BA = "
cat predictions/*_ws.txt | grep "^41" | cut -f 4 | datamash mean 1
