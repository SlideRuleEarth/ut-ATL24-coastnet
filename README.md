# Dependencies

``` bash
dnf -y install \
    cmake \
    cppcheck \
    opencv-devel \
    python3.9  \
    parallel \
    gmp-devel \
    mlpack-devel \
    mlpack-bin \
    gdal-devel \
    armadillo-devel
dnf config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/rhel8/x86_64/cuda-rhel8.repo
dnf -y install cuda
```
# Environment

source /opt/rh/gcc-toolset-12/enable
CUDACXX=/usr/local/cuda/bin/nvcc

# Build

``` bash
$ python3.9 -m venv venv
$ source ./venv/bin/activate
$ python --version
python --version
Python 3.9.18
$ python -m pip install --upgrade pip
...
Successfully installed pip-23.3.2
$ pip install torch numpy
Collecting torch
  Downloading torch-2.1.2-cp39-cp39-manylinux1_x86_64.whl (670.2 MB)
...
Successfully installed MarkupSafe-2.1.4, ...
```
