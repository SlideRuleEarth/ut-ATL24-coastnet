# Coastal Deep Network

Deep residual network for coastal feature extraction

# Dependencies

``` bash
dnf -y install \
    gcc \
    cmake \
    cppcheck \
    python3.9  \
    parallel \
    gmp-devel
```

You will also need to install the XGBoost `C Package`, found here:

`https://xgboost.readthedocs.io/en/stable/tutorials/c_api_tutorial.html#install-xgboost-on-conda-environment`

If you are not using Conda, you can simply install the package at the
system level.

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

After training, a model will be written to `./coastnet_model.json`.

This model will be used for classification and cross-validation.

If you would like to keep the model, you can copy it to the `./models`
directory with a date attached, like, for example,
`./models/coastnet_model-20240624.json`. You can then check the model
into source code control for later use.

NOTE that there have been problems using Large File Support (LFS) on
the SlideRule github repository, so LFS has been turned OFF for model
files.

Classify:

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
...
```

Classifed CSV files will be written to a `predictions` directory. Each
input granule CSV will have a corresponding classified `.csv` file and
a `.txt` file containing performance statistics.

``` bash
$ ls -1 predictions/
...
ATL03_20230213042035_08341807_006_01_gt2l_0_classified.csv
ATL03_20230213042035_08341807_006_01_gt2l_0_results.txt
ATL03_20230328030558_01031906_006_01_gt2l_0_classified.csv
ATL03_20230328030558_01031906_006_01_gt2l_0_results.txt
ATL03_20230407113920_02611908_006_01_gt1l_0_classified.csv
ATL03_20230407113920_02611908_006_01_gt1l_0_results.txt
ATL03_20230407113920_02611908_006_01_gt2l_0_classified.csv
ATL03_20230407113920_02611908_006_01_gt2l_0_results.txt
ATL03_20230407113920_02611908_006_01_gt3l_0_classified.csv
ATL03_20230407113920_02611908_006_01_gt3l_0_results.txt
$ cat predictions/ATL03_20230407113920_02611908_006_01_gt3l_0_results.txt
cls     acc     F1      bal_acc cal_F1  tp      tn      fp      fn      support total
0       0.983   0.867   0.924   0.914   26346   448235  3638    4464    30810   482683
40      0.998   0.952   0.981   0.980   11044   470520  696     423     11467   482683
41      0.985   0.992   0.951   0.913   437142  38460   3817    3264    440406  482683
weighted_accuracy = 0.985
weighted_F1 = 0.983
weighted_bal_acc = 0.950
weighted_cal_F1 = 0.915
```

Cross-validate:

``` bash
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

The `.txt` files contain statistics for the model that was trained on
all files. This is the model that will ultimately be shipped.

Typically, when the cross-validated statistics are worse than the
shipping model's statistics, it means that the model is over-fitting.
However, in this case, we expect the cross-validate statistics to be
(slightly) worse than the statistics for the shipping model: each
cross-validation fold can't quite represent the entire set of training
files, so the statistics for each fold will usually be equal to, or
slightly worse than, the shipping model's statistics.


Another way to put it is this: We want the shipping model to be as
good as it can possibly be, so we train it on all available training
data. This means that the shipping model can't be cross-validated
because if we had more labeled data to test upon, those
files would be part of the training dataset.

Here is an example:

```
$ make classify
...
Surface
Average Acc = 0.952
Average F1 = 0.962
Average BA = 0.933
...
Bathy
Average Acc = 0.982
Average F1 = 0.627
Average BA = 0.921
...

$ make xval
...
Surface
Average Acc = 0.944
Average F1 = 0.954
Average BA = 0.925
...
Bathy
Average Acc = 0.980
Average F1 = 0.603
Average BA = 0.893
```
