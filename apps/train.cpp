#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/custom_dataset.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/raster.h"
#include "network.h"
#include "xgboost.h"
#include "train_cmd.h"

const std::string usage {"ls *.csv | resnet [options]"};

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_coastnet;

    try
    {
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

        // Read input filenames
        vector<string> fns;

        if (args.verbose)
            clog << "Reading filenames from stdin" << endl;

        // Read filenames from stdin
        for (string line; getline(std::cin, line); )
            fns.push_back (line);

        if (args.verbose)
            clog << fns.size () << " filenames read" << endl;

        // Create the random number generator
        mt19937_64 rng (args.random_seed);

        // Split into train/test datasets
        shuffle (fns.begin (), fns.end (), rng);
        vector<string> train_filenames;
        vector<string> test_filenames;

        // If train/test split is 0.0, train on everything and test on everything
        if (args.train_test_split == 0.0)
        {
            train_filenames = fns;
            test_filenames = fns;
        }
        else
        {
            for (size_t i = 0; i < fns.size (); ++i)
            {
                const size_t total_datasets = std::ceil (1.0 / args.train_test_split);
                // To which dataset does 'i' belong?
                size_t d = i * total_datasets / fns.size ();
                assert (d < total_datasets);
                if (d == args.test_dataset)
                    test_filenames.push_back (fns[i]);
                else
                    train_filenames.push_back (fns[i]);
            }
        }

        if (args.verbose)
        {
            clog << "###############################" << endl;
            clog << "Training files" << endl;
            for (auto fn : train_filenames)
                clog << fn << endl;
            clog << train_filenames.size () << " total train files" << endl;
            clog << "###############################" << endl;
            clog << "Testing files" << endl;
            for (auto fn : test_filenames)
                clog << fn << endl;
            clog << test_filenames.size () << " total test files" << endl;
            clog << "###############################" << endl;
        }

        // Always dump testing files to stdout
        for (auto fn : test_filenames)
            cout << fn << endl;

        // Params
        hyper_params hp;
        augmentation_params ap;
        const bool enable_augmentation = true;

        if (args.verbose)
        {
            clog << "sampling parameters:" << endl;
            print_sampling_params (clog);
            clog << "hyper parameters:" << endl;
            clog << hp << endl;
            clog << "augmentation parameters:" << endl;
            clog << ap << endl;
            clog << "Creating dataset" << endl;
        }

        // Create Datasets
        const size_t training_samples_per_class = 200'000;
        const size_t test_samples_per_class = 20'000;
        auto train_dataset = coastnet_dataset (train_filenames,
            sampling_params::patch_rows,
            sampling_params::patch_cols,
            sampling_params::aspect_ratio,
            ap,
            enable_augmentation,
            training_samples_per_class,
            args.verbose,
            rng);
        auto test_dataset = coastnet_dataset (test_filenames,
            sampling_params::patch_rows,
            sampling_params::patch_cols,
            sampling_params::aspect_ratio,
            ap,
            false, // enable augmentation
            test_samples_per_class,
            false, // args.verbose,
            rng);

        if (args.verbose)
        {
            clog << train_dataset.size ().value () << " total train samples" << endl;
            clog << test_dataset.size ().value () << " total test samples" << endl;
        }

        const auto total_train_samples = train_dataset.size().value();
        const auto total_test_samples = test_dataset.size().value();

        clog << "total_train_samples " << total_train_samples << endl;
        clog << "total_test_samples " << total_test_samples << endl;

        // Create Data Loaders
        auto train_loader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler> (
            std::move(train_dataset),
            hp.batch_size);
        auto test_loader = torch::data::make_data_loader<torch::data::samplers::SequentialSampler> (
            std::move(test_dataset),
            hp.batch_size);

        // Always use CUDA during training
        //
        // Create the hardware device
        torch::Device device (torch::kCUDA);

        // Create the network
        Network network (args.num_classes);

        // Print the network
        clog << network << endl;

        clog << "Training network" << endl;
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
                const auto samples = batch.data.sizes()[0];

                at::autocast::set_enabled(true);
                auto data = batch.data.view({samples, -1}).to(device);
                auto target = batch.target.squeeze().to(device);
                auto output = network->forward(data);
                at::autocast::clear_cache();
                at::autocast::set_enabled(false);
                auto prediction = output.argmax(1);
                auto loss = torch::nn::functional::cross_entropy(output, target);

                show_samples ("input", data, sampling_params::patch_rows, sampling_params::patch_cols);

                // Update number of correctly classified samples
                total_correct += prediction.eq(target).sum().item<int64_t>();

                static int timeout = 1;
                const auto ch = cv::waitKey (timeout);

                switch (ch)
                {
                    default:
                    break;
                    case 27: // quit
                    return 0;
                    break;
                    case 32: // pause/unpause
                    timeout = !timeout;
                    break;
                }

                if (batch_index == 0)
                {
                    clog << "epoch: " << epoch;
                    clog << " loss " << loss.item<float>();
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

            // Save the model
            torch::save (network, args.network_filename);
        }

        clog << "Done training" << endl;

        // Test the network
        network->train (false);
        network->to (device);

        // Disabling gradient calculation is useful for inference,
        // when you are sure that you will not call Tensor::backward.
        // It will reduce memory consumption and speed up computations
        // that would otherwise have requires_grad() == true.
        torch::NoGradGuard no_grad;

        size_t batch_index = 0;
        size_t total_correct = 0;

        clog << "Testing" << endl;

        for (auto &batch : *test_loader)
        {
            const auto samples = batch.data.sizes()[0];
            auto data = batch.data.view({samples, -1}).to(device);
            auto target = batch.target.squeeze().to(device);
            auto output = network->forward(data);
            auto prediction = output.argmax(1);
            auto loss = torch::nn::functional::cross_entropy(output, target);
            show_samples ("testing", data, sampling_params::patch_rows, sampling_params::patch_cols);
            cv::waitKey (1);

            // Update number of correctly classified samples
            total_correct += prediction.eq(target).sum().item<int64_t>();

            clog << "batch index: " << batch_index;
            clog << " loss " << loss.item<float>();
            clog << endl;

            ++batch_index;
        }

        auto accuracy = static_cast<double>(total_correct) / total_test_samples;
        clog << "total_correct " << total_correct << endl;
        clog << "total_test_samples " << total_test_samples << endl;
        clog << "accuracy " << accuracy << endl;

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
