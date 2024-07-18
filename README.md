# Coastal Deep Network

Deep residual network for coastal feature extraction

# Dependencies

``` bash
dnf -y install \
    cmake \
    cppcheck \
    python3.9  \
    parallel \
    gmp-devel
```

You will also need to install the XGBoost `C Package`, found here:

`https://xgboost.readthedocs.io/en/stable/tutorials/c_api_tutorial.html#install-xgboost-on-conda-environment`

If you are not using Conda, you can simply install the package at the system level.

# Build

Build the executables:

``` bash
$ make build test
```

Create symlink to the input data:

``` bash
$ ln -s <path to training data>/data
```

Create the model:

``` bash
$ make train
...
cmd_line_parameters:
help: false
verbose: true
random-seed: 123
model-filename: coastnet_model.json
train-test-split: 0
epochs: 100
test-dataset: 0
num-classes: 7
Reading filenames from stdin
180 filenames read
###############################
Training files
./input/manual/ATL03_20221012141554_03341701_005_01_gt1r_0.csv
./input/manual/ATL03_20200827111933_09640801_005_01_gt3l_0.csv
./input/manual/ATL03_20220808201030_07321602_006_01_gt1r_0.csv
...
```

Classify and cross validate:

``` bash
$ make classify
...
Creating booster
cmd_line_parameters:
help: false
verbose: true
use_predictions: false
num-classes: 7
model-filename: coastnet_model.json
results-filename: predictions/ATL03_20181027185143_04450108_005_01_gt3r_0_results.txt

$ make xval
...
Reading filenames from stdin
180 filenames read
###############################
Training files
./input/manual/ATL03_20190502032012_05170308_005_01_gt3l_0.csv
./input/manual/ATL03_20181222024630_12900108_005_01_gt3r_0.csv
./input/manual/ATL03_20210925114800_00461308_005_01_gt3l_0.csv
...
144 total train files
###############################
Testing files
./input/manual/ATL03_20221012141554_03341701_005_01_gt1r_0.csv
./input/manual/ATL03_20200827111933_09640801_005_01_gt3l_0.csv
./input/manual/ATL03_20220808201030_07321602_006_01_gt1r_0.csv
./input/manual/ATL03_20210325203928_00161102_005_01_gt1r_0.csv
./input/manual/ATL03_20210402122421_01331101_005_01_gt1r_0.csv
...
36 total test files
###############################
sampling parameters:
patch_rows: 63
patch_cols: 15
input_size: 945
aspect_ratio: 4
augmentation parameters:
jitter_x_std: 0.0001
jitter_z_std: 0.1
scale_x_min: 0.9
scale_x_max: 1.1
scale_z_min: 0.9
scale_z_max: 1.1
mirror_probabilty: 0.5
...
```
