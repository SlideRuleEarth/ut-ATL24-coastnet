#pragma once

#include "ATL24_coastnet/precompiled.h"

namespace ATL24_coastnet
{

namespace sampling_params
{
    const size_t patch_rows = 63;
    const size_t patch_cols = 7;
    const int64_t input_size = patch_rows * patch_cols;
    const double aspect_ratio = 8.0;
}

void print_sampling_params (std::ostream &os)
{
    os << "patch_rows: " << sampling_params::patch_rows << std::endl;
    os << "patch_cols: " << sampling_params::patch_cols << std::endl;
    os << "input_size: " << sampling_params::input_size << std::endl;
    os << "aspect_ratio: " << sampling_params::aspect_ratio << std::endl;
}

struct hyper_params
{
    int64_t batch_size = 64;
    double initial_learning_rate = 0.01;
};

std::ostream &operator<< (std::ostream &os, const hyper_params &hp)
{
    os << "batch_size: " << hp.batch_size << std::endl;
    os << "initial_learning_rate: " << hp.initial_learning_rate << std::endl;
    return os;
}

} // namespace ATL24_coastnet
