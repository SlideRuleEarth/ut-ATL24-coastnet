#include <torch/torch.h>
#include <cassert>
#include <iostream>

int main()
{
    using namespace std;

    // Check if assertions are enabled
    bool is_debug = false;
    assert (is_debug = true);
    clog << "Debug is " << (is_debug ? "ON" : "OFF") << endl;

    // Check if we have GPU support
    const auto cuda_available = torch::cuda::is_available();
    clog << (cuda_available
        ? "CUDA available. Training on GPU."
        : "CUDA not available. Training on CPU.") << '\n';
}
