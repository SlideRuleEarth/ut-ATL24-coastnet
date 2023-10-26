# Bash strict mode
set -euo pipefail
IFS=$'\n\t'

echo VIRTUAL_ENV=$VIRTUAL_ENV

# Get the path to libtorch
cmake_prefix_path=$(python -c 'import torch;print(torch.utils.cmake_prefix_path)')
echo cmake_prefix_path=${cmake_prefix_path}

# Put builds in separate directories
mkdir -p ./build/debug
mkdir -p ./build/release

# Generate the Makefiles
pushd ./build/debug
cmake -D CMAKE_BUILD_TYPE=Debug -D CMAKE_PREFIX_PATH=${cmake_prefix_path} ../..
popd

pushd ./build/release
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_PREFIX_PATH=${cmake_prefix_path} ../..
popd
