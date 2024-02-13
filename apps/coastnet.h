#pragma once

#include <iostream>

namespace ATL24_resnet
{

struct hyper_params
{
    size_t patch_rows = 64;
    size_t patch_cols = 64;
    int64_t input_size = patch_rows * patch_cols;
    double aspect_ratio = 20.0;
    int64_t batch_size = 64;
    double initial_learning_rate = 0.01;
};

std::ostream &operator<< (std::ostream &os, const hyper_params &hp)
{
    os << "patch_rows: " << hp.patch_rows << std::endl;
    os << "patch_cols: " << hp.patch_cols << std::endl;
    os << "input_size: " << hp.input_size << std::endl;
    os << "aspect_ratio: " << hp.aspect_ratio << std::endl;
    os << "batch_size: " << hp.batch_size << std::endl;
    os << "initial_learning_rate: " << hp.initial_learning_rate << std::endl;
    return os;
}

torch::nn::Conv2d conv3x3 (int64_t in_channels, int64_t out_channels, int64_t stride = 1)
{
    return torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, 3)
        .stride(stride)
        .padding(1)
        .bias(false));
}

class ResidualBlockImpl : public torch::nn::Module
{
    public:
    ResidualBlockImpl (
        int64_t in_channels,
        int64_t out_channels,
        int64_t stride = 1,
        torch::nn::Sequential downsample = nullptr)
        : conv1(conv3x3(in_channels, out_channels, stride))
        , bn1(out_channels)
        , conv2(conv3x3(out_channels, out_channels))
        , bn2(out_channels)
        , downsampler(downsample)
    {
        register_module("conv1", conv1);
        register_module("bn1", bn1);
        register_module("relu", relu);
        register_module("conv2", conv2);
        register_module("bn2", bn2);

        if (downsampler) {
            register_module("downsampler", downsampler);
        }
    }

    torch::Tensor forward(torch::Tensor x)
    {
        auto out = conv1->forward(x);
        out = bn1->forward(out);
        out = relu->forward(out);
        out = conv2->forward(out);
        out = bn2->forward(out);

        auto residual = downsampler ? downsampler->forward(x) : x;
        out += residual;
        out = relu->forward(out);

        return out;
    }

    private:
    torch::nn::Conv2d conv1;
    torch::nn::BatchNorm2d bn1;
    torch::nn::ReLU relu;
    torch::nn::Conv2d conv2;
    torch::nn::BatchNorm2d bn2;
    torch::nn::Sequential downsampler;
};

TORCH_MODULE(ResidualBlock);

template<typename Block>
class ResNetImpl : public torch::nn::Module {
 public:
    explicit ResNetImpl(const std::array<int64_t, 3>& layers, int64_t num_classes = 10);
    torch::Tensor forward(torch::Tensor x);

 private:
    int64_t in_channels = 16;
    torch::nn::Conv2d conv{conv3x3(1, 16)};
    torch::nn::BatchNorm2d bn{16};
    torch::nn::ReLU relu;
    torch::nn::Sequential layer1;
    torch::nn::Sequential layer2;
    torch::nn::Sequential layer3;
    torch::nn::AvgPool2d avg_pool{10};
    torch::nn::Linear fc;

    torch::nn::Sequential make_layer(int64_t out_channels, int64_t blocks, int64_t stride = 1);
};

template<typename Block>
ResNetImpl<Block>::ResNetImpl(const std::array<int64_t, 3>& layers, int64_t num_classes) :
    layer1(make_layer(16, layers[0])),
    layer2(make_layer(32, layers[1], 2)),
    layer3(make_layer(64, layers[2], 2)),
    fc(64, num_classes) {
    register_module("conv", conv);
    register_module("bn", bn);
    register_module("relu", relu);
    register_module("layer1", layer1);
    register_module("layer2", layer2);
    register_module("layer3", layer3);
    register_module("avg_pool", avg_pool);
    register_module("fc", fc);
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

template<typename Block>
torch::Tensor ResNetImpl<Block>::forward(torch::Tensor x) {
    //print_shape ("\nX", x);
    auto out = conv->forward(x);
    //print_shape ("conv", out);
    out = bn->forward(out);
    //print_shape ("bn", out);
    out = relu->forward(out);
    //print_shape ("relu", out);
    out = layer1->forward(out);
    //print_shape ("layer1", out);
    out = layer2->forward(out);
    //print_shape ("layer2", out);
    out = layer3->forward(out);
    //print_shape ("layer3", out);
    out = avg_pool->forward(out);
    //print_shape ("avg", out);
    out = out.view({out.size(0), -1});
    //print_shape ("view", out);
    out = fc->forward(out);
    //print_shape ("fc", out);
    return out;
}

template<typename Block>
torch::nn::Sequential ResNetImpl<Block>::make_layer(int64_t out_channels, int64_t blocks, int64_t stride) {
    torch::nn::Sequential layers;
    torch::nn::Sequential downsample{nullptr};

    if (stride != 1 || in_channels != out_channels) {
        downsample = torch::nn::Sequential{
            conv3x3(in_channels, out_channels, stride),
            torch::nn::BatchNorm2d(out_channels)
        };
    }

    layers->push_back(Block(in_channels, out_channels, stride, downsample));

    in_channels = out_channels;

    for (int64_t i = 1; i != blocks; ++i) {
        layers->push_back(Block(out_channels, out_channels));
    }

    return layers;
}

template<typename Block = ResidualBlock>
class ResNet : public torch::nn::ModuleHolder<ResNetImpl<Block>> {
 public:
    using torch::nn::ModuleHolder<ResNetImpl<Block>>::ModuleHolder;
};

} // namespace ATL24_resnet
