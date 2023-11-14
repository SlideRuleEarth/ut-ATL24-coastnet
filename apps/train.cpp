#include "ATL24_resnet/precompiled.h"
#include "ATL24_resnet/custom_dataset.h"
#include "ATL24_resnet/utils.h"
#include "viper/raster.h"
#include "resnet.h"
#include "train_cmd.h"

const std::string usage {"ls *.csv | resnet [options]"};

cv::Mat to_mat (torch::Tensor t)
{
    using namespace std;

    const int w = t.sizes()[0];
    const int h = t.sizes()[1];
    cv::Mat m (cv::Size {h, w}, CV_8UC1, t.data_ptr<unsigned char>());
    cv::normalize (m, m, 0, 255, cv::NORM_MINMAX);
    return m;
}

void show_samples (
    const std::string &title,
    const torch::Tensor &samples,
    const int patch_rows,
    const int patch_cols,
    int total = 10)
{
    using namespace std;

    // Make sure there are enough samples
    total = std::min (total, int (samples.sizes()[0]));

    // Convert to uint8 Mat
    auto t = torch::zeros({total, patch_rows, patch_cols}, torch::kByte);
    for (int i = 0; i < total; ++i)
        t[i] = samples[i].reshape ({patch_rows, patch_cols}).toType (torch::kByte);

    t = t.permute ({1, 0, 2}).reshape ({patch_rows, total * patch_cols});

    cv::imshow(title, to_mat (t));
}

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_resnet;

    try
    {
        // Check if assertions are enabled
        bool is_debug = false;
        assert (is_debug = true);
        clog << "Debug is " << (is_debug ? "ON" : "OFF") << endl;

        // Check if we have GPU support
        const auto cuda_available = torch::cuda::is_available();
        clog << (cuda_available
            ? "CUDA available. Training on GPU."
            : "CUDA not available. Training on CPU.") << '\n';

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

        // Read input filenames
        vector<string> fns;

        if (args.verbose)
            clog << "Reading filenames from stdin" << endl;

        // Read filenames from stdin
        for (string line; getline(std::cin, line); )
            fns.push_back (line);

        if (args.verbose)
            clog << fns.size () << " filenames read" << endl;

        // Split into train/test datasets
        default_random_engine rng (args.random_seed);
        shuffle (fns.begin (), fns.end (), rng);
        vector<string> train_filenames;
        vector<string> test_filenames;

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

        // Get hyper-parameters
        hyper_params hp;

        // Get other parameters
        regularization_params rp;

        clog << "hyper_parameters:" << endl;
        clog << hp << endl;

        // Create Datasets
        const size_t training_samples_per_class = 10'000;
        const size_t test_samples_per_class = 200;
        auto train_dataset = classified_point_dataset (train_filenames,
            hp.patch_rows,
            hp.patch_cols,
            hp.aspect_ratio,
            rp,
            training_samples_per_class,
            false, // args.verbose,
            rng)
            .map(torch::data::transforms::Stack<>());
        auto test_dataset = classified_point_dataset (test_filenames,
            hp.patch_rows,
            hp.patch_cols,
            hp.aspect_ratio,
            rp,
            test_samples_per_class,
            false, // args.verbose,
            rng)
            .map(torch::data::transforms::Stack<>());

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

        // Create the hardware device
        torch::Device device (cuda_available ? torch::kCUDA : torch::kCPU);

        // Create the network
        std::array<int64_t, 3> layers{2, 2, 2};
        ResNet<ResidualBlock> network (layers, args.num_classes);

        // Print the network
        //clog << network << endl;

        clog << "Training network" << endl;
        network->to (device);
        network->train (true);

        //torch::optim::Adam optimizer (network->parameters (), torch::optim::AdamOptions (hp.initial_learning_rate));
        torch::optim::SGD optimizer (network->parameters (), torch::optim::SGDOptions (hp.initial_learning_rate));
        torch::optim::StepLR scheduler (optimizer, 10, 0.1);

        for (size_t epoch = 0; epoch < hp.epochs; ++epoch)
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
                show_samples ("input", data, hp.patch_rows, hp.patch_cols);

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
        }

        // Save the model
        torch::save (network, args.network_filename);

        // Test the network
        network->train (false);
        network->to (torch::kCPU);

        // Disabling gradient calculation is useful for inference,
        // when you are sure that you will not call Tensor::backward.
        // It will reduce memory consumption and speed up computations
        // that would otherwise have requires_grad() == true.
        torch::NoGradGuard no_grad;

        size_t batch_index = 0;
        size_t total_correct = 0;

        for (auto &batch : *test_loader)
        {
            auto data = batch.data.unsqueeze(0).permute({1, 0, 2, 3});
            auto target = batch.target.squeeze();
            auto output = network->forward(data);
            auto prediction = output.argmax(1);
            auto loss = torch::nn::functional::cross_entropy(output, target);
            show_samples ("testing", data, hp.patch_rows, hp.patch_cols);
            cv::waitKey (1);

            // Update number of correctly classified samples
            total_correct += prediction.eq(target).sum().item<int64_t>();

            clog << "batch index: " << batch_index;
            clog << "/" << total_test_samples;
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
