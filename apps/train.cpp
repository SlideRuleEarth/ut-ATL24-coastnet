#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/custom_dataset.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/raster.h"
#include "sampling.h"
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

        vector<string> fns;

        if (args.verbose)
            clog << "Reading filenames from stdin" << endl;

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
        augmentation_params ap;
        const bool enable_augmentation = true;

        if (args.verbose)
        {
            clog << "sampling parameters:" << endl;
            print_sampling_params (clog);
            clog << "augmentation parameters:" << endl;
            clog << ap << endl;
            clog << "Creating datasets" << endl;
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
            clog << train_dataset.size () << " total train samples" << endl;
            clog << test_dataset.size () << " total test samples" << endl;
        }

        const auto total_train_samples = train_dataset.size();
        const auto total_test_samples = test_dataset.size();

        clog << "Total train samples " << total_train_samples << endl;
        clog << "Total test samples " << total_test_samples << endl;

        // Create the raw data that gets passed to xgbooster
        features train_features (train_dataset);
        features test_features (test_dataset);

        // Dump class counts of training set
        {
        unordered_map<size_t,size_t> counts;
        for (size_t i = 0; i < train_dataset.size (); ++i)
            ++counts[train_dataset.get_label (i)];

        clog << "Train labels:" << endl;
        for (auto i : counts)
            clog << i.first << "\t" << i.second << endl;
        }

        // Dump class counts of test set
        {
        unordered_map<size_t,size_t> counts;
        for (size_t i = 0; i < test_dataset.size (); ++i)
            ++counts[test_dataset.get_label (i)];

        clog << "Test labels:" << endl;
        for (auto i : counts)
            clog << i.first << "\t" << i.second << endl;
        }

        // Dump class counts of training set
        {
        const auto labels = train_features.get_labels ();
        unordered_map<size_t,size_t> counts;
        for (size_t i = 0; i < labels.size (); ++i)
            ++counts[labels[i]];

        clog << "Train labels:" << endl;
        for (auto i : counts)
            clog << i.first << "\t" << i.second << endl;
        }

        // Dump class counts of test set
        {
        const auto labels = test_features.get_labels ();
        unordered_map<size_t,size_t> counts;
        for (size_t i = 0; i < labels.size (); ++i)
            ++counts[labels[i]];

        clog << "Test labels:" << endl;
        for (auto i : counts)
            clog << i.first << "\t" << i.second << endl;
        }

        clog << "Training model" << endl;

        // Create the booster
        xgboost::xgbooster xgb (args.verbose);

        const size_t train_rows = train_features.size ();
        const size_t train_cols = train_features.features_per_sample ();

        xgb.train (train_features.get_features (),
            train_features.get_labels (),
            train_rows,
            train_cols,
            args.epochs);

        clog << "Testing model" << endl;

        const size_t test_rows = test_features.size ();
        const size_t test_cols = test_features.features_per_sample ();
        const auto predictions = xgb.predict (test_features.get_features (),
            test_rows,
            test_cols);

        double total_correct = 0;

        const auto labels = test_features.get_labels ();
        assert (labels.size () == predictions.size ());
        for (size_t i = 0; i < labels.size (); ++i)
            total_correct += (labels[i] == predictions[i]);

        unordered_map<size_t,size_t> prediction_counts;
        for (size_t i = 0; i < predictions.size (); ++i)
            ++prediction_counts[predictions[i]];

        clog << "Predictions:" << endl;
        for (auto i : prediction_counts)
            clog << i.first << "\t" << i.second << endl;

        const double accuracy = total_correct / predictions.size ();

        if (args.verbose)
        {
            clog << "Total predictions = " << predictions.size () << endl;
            clog << "Total correct = " << total_correct << endl;
            clog << "Training accuracy = " << 100.0 * accuracy << "%" << endl;
        }

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
