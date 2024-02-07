# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

mkdir -p predictions
for n in $(seq 0 4)
do
    parallel --lb --dry-run \
        "build/debug/classify --verbose --num-classes=7 --network-filename=resnet_network-${n}.pt --results-filename=predictions/{/.}_results-${n}.txt < {} > predictions/{/.}_classified_${n}.csv" \
        :::: resnet_test_files-${n}.txt
done
