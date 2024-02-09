#include "ATL24_resnet/precompiled.h"
#include "ATL24_resnet/custom_dataset.h"
#include "ATL24_resnet/utils.h"
#include "viper/raster.h"
#include "coastnet.h"
#include "train_coastnet_cmd.h"

const std::string usage {"ls *.csv | resnet [options]"};

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_resnet;

    try
    {
        // Check if we have GPU support
        const auto cuda_available = torch::cuda::is_available();
        if (!cuda_available)
            throw runtime_error ("Could not find GPU");

        // Parse the args
        const auto args = cmd::get_args (argc, argv, usage);

        // If you are getting help, exit without an error
        if (args.help)
            return 0;

        if (args.verbose)
        {
            // Show the args
            clog << "cmd_line_parameters:" << endl;
            clog << args << endl;
        }

        // Set the seed in torch
        torch::manual_seed (args.random_seed);

        vector<string> fns;

        if (args.verbose)
            clog << "Reading filenames from stdin" << endl;

        for (string line; getline(std::cin, line); )
            fns.push_back (line);

        if (args.verbose)
        {
            clog << fns.size () << " filenames read" << endl;
            clog << "Training files" << endl;
            for (auto fn : fns)
                clog << fn << endl;
            clog << fns.size () << " total training files" << endl;
        }

        // Create pseudo-RNG
        default_random_engine rng (args.random_seed);

        // Get hyper-parameters
        hyper_params hp;

        // Get other parameters
        augmentation_params ap {
            true, // enabled
            0.1, // jitter_std standard deviation in meters
            0.9, // scale_x_min meters
            1.1, // scale_x_max meters
            0.9, // scale_z_min meters
            1.1, // scale_z_max meters
            true
        };

        if (args.verbose)
        {
            clog << "hyper parameters:" << endl;
            clog << hp << endl;
            clog << "augmentation parameters:" << endl;
            clog << ap << endl;
            clog << "Creating dataset" << endl;
        }

        auto train_dataset = coastnet_dataset (fns,
            hp.patch_rows,
            hp.patch_cols,
            hp.aspect_ratio,
            ap,
            args.max_samples_per_class,
            args.verbose,
            rng)
            .map(torch::data::transforms::Stack<>());

        const auto total_train_samples = train_dataset.size().value();

        // Create Data Loader
        auto train_loader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler> (
            std::move(train_dataset),
            hp.batch_size);

        // Create the hardware device
        torch::Device device (cuda_available ? torch::kCUDA : torch::kCPU);

        // Create the network
        const size_t num_classes = 3;
        std::array<int64_t, 3> layers{2, 2, 2};
        ResNet<ResidualBlock> network (layers, num_classes);

        network->to (device);
        network->train (true);

        torch::optim::SGD optimizer (network->parameters (), torch::optim::SGDOptions (hp.initial_learning_rate));
        torch::optim::StepLR scheduler (optimizer, 10, 0.1);

        for (size_t epoch = 0; epoch < args.epochs; ++epoch)
        {
            size_t batch_index = 0;

            if (args.verbose)
            {
                for (const auto &p : optimizer.param_groups ())
                    clog << "Optimizer learning rate = "<< p.options ().get_lr () << endl;
            }

            size_t total_correct = 0;

            for (auto &batch : *train_loader)
            {
                auto data = batch.data.unsqueeze(0).permute({1, 0, 2, 3}).to(device);
                auto target = batch.target.squeeze().to(device);
                auto output = network->forward(data);
                auto prediction = output.argmax(1);
                auto loss = torch::nn::functional::cross_entropy(output, target);

                // Update number of correctly classified samples
                total_correct += prediction.eq(target).sum().item<int64_t>();

                if (static_cast<int> (batch_index + 1) == hp.batch_size)
                {
                    clog << "epoch: " << epoch + 1;
                    clog << " of " << args.epochs;
                    clog << " loss " << loss.item<float>() << endl;
                    clog << endl;
                }

                // Zero gradients
                optimizer.zero_grad();
                // Compute gradients
                loss.backward ();
                // Update parameters
                optimizer.step ();

                ++batch_index;
            }

            auto accuracy = static_cast<double>(total_correct) / total_train_samples;
            clog << "total_correct " << total_correct << endl;
            clog << "total_train_samples " << total_train_samples << endl;
            clog << "accuracy " << accuracy << endl;

            scheduler.step();

            clog << "Saving model..." << endl;

            // Save the model
            torch::save (network, args.network_filename);
        }

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
