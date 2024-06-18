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

template<typename T,typename U>
void print_shape (const T &title, const U &x)
{
    using namespace std;
    clog << title;
    for (size_t i = 0; i < x.sizes().size (); ++i)
        clog << '\t' << x.sizes()[i];
    clog << endl;
}

class NetworkImpl : public torch::nn::Module
{
    public:
    explicit NetworkImpl (size_t num_classes)
        : fc1(sampling_params::input_size, hidden_size1)
        , fc2(hidden_size1, hidden_size2)
        , fc3(hidden_size2, num_classes)
    {
        register_module ("fc1", fc1);
        register_module ("fc2", fc2);
        register_module ("fc3", fc3);
    }
    torch::Tensor forward (torch::Tensor x)
    {
        x = fc1->forward (x);
        x = torch::nn::functional::relu (x);
        x = fc2->forward (x);
        x = torch::nn::functional::relu (x);
        x = fc3->forward (x);
        return x;
    }

    private:
    const size_t hidden_size1 = 64;
    const size_t hidden_size2 = 32;
    torch::nn::Linear fc1 = nullptr;
    torch::nn::Linear fc2 = nullptr;
    torch::nn::Linear fc3 = nullptr;
};

TORCH_MODULE(Network);

class CNNImpl : public torch::nn::Module
{
    public:
    explicit CNNImpl(int64_t num_classes)
        : dense_layer(32 * 15, num_classes)
    {
        register_module("conv_block1", conv_block1);
        register_module("conv_block2", conv_block2);
        register_module("dense_layer", dense_layer);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        x = conv_block1->forward(x);
        x = conv_block2->forward(x);
        x = x.view({-1, 32 * 15});
        x = dense_layer->forward(x);
        return x;
    }

    private:
    torch::nn::Sequential conv_block1
    {
        torch::nn::Conv2d(torch::nn::Conv2dOptions(1, 16, 5).stride(1).padding(2)),
        torch::nn::BatchNorm2d(16), torch::nn::ReLU(),
        torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions(2).stride(2))
    };
    torch::nn::Sequential conv_block2
    {
        torch::nn::Conv2d(torch::nn::Conv2dOptions(16, 32, 5).stride(1).padding(2)),
        torch::nn::BatchNorm2d(32), torch::nn::ReLU(),
        torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions(2).stride(2))
    };
    torch::nn::Linear dense_layer;
};

TORCH_MODULE(CNN);

} // namespace ATL24_coastnet