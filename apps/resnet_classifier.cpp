#include "ATL24_resnet/precompiled.h"
#include "ATL24_resnet/confusion.h"
#include "ATL24_resnet/utils.h"
#include "viper/raster.h"
#include "resnet.h"
#include "resnet_classifier_cmd.h"

const std::string usage {"ls *.csv | resnet [options]"};

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

        // Get hyper-parameters
        hyper_params hp;

        // Create the network
        std::array<int64_t, 3> layers{2, 2, 2};
        ResNet<ResidualBlock> network (layers, args.num_classes);

        // Check the network
        if (args.verbose)
            clog << "Reading " << args.network_filename << endl;

        {
            ifstream ifs (args.network_filename);

            if (!ifs)
                throw runtime_error ("Error loading network from disk");
        }

        // Load the network
        torch::load (network, args.network_filename);

        // Check if we have GPU support
        const auto cuda_available = torch::cuda::is_available();
        clog << (cuda_available
            ? "CUDA is available."
            : "CUDA is NOT available.") << endl;

        // Create the hardware device
        //torch::Device device (cuda_available ? torch::kCUDA : torch::kCPU);
        torch::Device device (torch::kCPU);

        // Prepare for inference
        network->train (false);
        network->to (device);

        // Disabling gradient calculation is useful for inference,
        // when you are sure that you will not call Tensor::backward.
        // It will reduce memory consumption and speed up computations
        // that would otherwise have requires_grad() == true.
        torch::NoGradGuard no_grad;

        // Read the points
        auto p = read_classified_point2d (cin);

        if (args.verbose)
        {
            clog << p.size () << " points read" << endl;
            clog << "Sorting points" << endl;
        }

        // Sort them by X
        sort (p.begin (), p.end (),
            [](const auto &a, const auto &b)
            { return a.x < b.x; });

        // Keep track of performance
        vector<confusion_matrix> cm (args.num_classes);

        if (args.verbose)
            clog << "Classifying points" << endl;

        // Copy the points
        auto q (p);

        // Set regularization parameters
        regularization_params rp;
        rp.enabled = false;
        default_random_engine rng (0);

        // For each point
#pragma omp parallel for
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Create the raster at this point
            auto r = create_raster (p, i, hp.patch_size, hp.patch_size, hp.aspect_ratio, rp, rng);

            // Create image Tensor from raster
            auto t = torch::from_blob (&r[0],
                { static_cast<int> (hp.patch_size), static_cast<int> (hp.patch_size) },
                torch::kUInt8).to(torch::kFloat);

            // [32 32] -> [1 1 32 32]
            t = t.unsqueeze(0).unsqueeze(0).to(device);

            // Decode
            auto output = network->forward (t);
            auto prediction = output.argmax(1);

            // Save true value, 0 in the raster means 'no value'
            const long actual = p[i].cls + 1;

            // Convert prediction from tensor to int
            const long pred = prediction[0].item<long> ();

            // Track performance
            for (long cls = 1; cls < static_cast<long> (cm.size ()); ++cls)
            {
                // Update the matrix
                const bool is_present = (actual == cls);
                const bool is_predicted = (pred == cls);
#pragma omp critical
                cm[cls].update (is_present, is_predicted);
            }

            // Save predicted value, 0 in the point cloud means noise
            assert (pred > 0);
            q[i].cls = pred - 1;
        }

        // Compile results
        stringstream ss;
        ss << setprecision(3) << fixed;
        ss << "cls"
            << "\t" << "acc"
            << "\t" << "F1"
            << "\t" << "bal_acc"
            << "\t" << "tp"
            << "\t" << "tn"
            << "\t" << "fp"
            << "\t" << "fn"
            << "\t" << "support"
            << "\t" << "total"
            << endl;
        double weighted_f1 = 0.0;
        double weighted_accuracy = 0.0;
        double weighted_bal_acc = 0.0;
        for (size_t i = 1; i < cm.size (); ++i)
        {
            ss << i
                << "\t" << cm[i].accuracy ()
                << "\t" << cm[i].F1 ()
                << "\t" << cm[i].balanced_accuracy ()
                << "\t" << cm[i].true_positives ()
                << "\t" << cm[i].true_negatives ()
                << "\t" << cm[i].false_positives ()
                << "\t" << cm[i].false_negatives ()
                << "\t" << cm[i].support ()
                << "\t" << cm[i].total ()
                << endl;
            if (!isnan (cm[i].F1 ()))
                weighted_f1 += cm[i].F1 () * cm[i].support () / cm[i].total ();
            if (!isnan (cm[i].accuracy ()))
                weighted_accuracy += cm[i].accuracy () * cm[i].support () / cm[i].total ();
            if (!isnan (cm[i].balanced_accuracy ()))
                weighted_bal_acc += cm[i].balanced_accuracy () * cm[i].support () / cm[i].total ();
        }
        ss << "weighted_F1 = " << weighted_f1 << endl;
        ss << "weighted_accuracy = " << weighted_accuracy << endl;
        ss << "weighted_bal_acc = " << weighted_bal_acc << endl;

        // Show results
        if (args.verbose)
            clog << ss.str ();

        // Write results, if specified
        if (!args.results_filename.empty ())
        {
            if (args.verbose)
                clog << "Writing " << args.results_filename << endl;

            ofstream ofs (args.results_filename);

            if (!ofs)
                cerr << "Could not open file for writing" << endl;
            else
                ofs << ss.str ();
        }

        // Write classified output to stdout
        write_classified_point2d (cout, q);

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
