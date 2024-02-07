# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

find ./data/local/3DGL/*.csv | build/debug/train_coastnet \
    --verbose \
    --network-filename=coastnet.pt
