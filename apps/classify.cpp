#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/dataframe.h"
#include "ATL24_coastnet/raster.h"
#include "network.h"
#include "classify_cmd.h"

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
            clog << "cmd_line_parameters:" << endl;
            clog << args;
        }

        // Params
        hyper_params hp;

        if (args.verbose)
        {
            clog << "sampling parameters:" << endl;
            print_sampling_params (clog);
            clog << endl;
            clog << "hyper parameters:" << endl;
            clog << hp << endl;
        }

        // Create the network
        Network network (args.num_classes);

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

        // Always use CPU during inference
        //
        // Create the hardware device
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
        const auto df = ATL24_coastnet::dataframe::read (cin);

        // Convert it to the correct format
        auto p = convert_dataframe (df);

        if (args.verbose)
        {
            clog << p.size () << " points read" << endl;
            clog << "Sorting points" << endl;
        }

        // Sort them by X
        sort (p.begin (), p.end (),
            [](const auto &a, const auto &b)
            { return a.x < b.x; });

        if (args.verbose)
            clog << "Classifying points" << endl;

        // Copy the points
        auto q (p);

        // Setup the prediction cache
        prediction_cache cache;

        // Keep track of cache usage
        size_t cache_lookups = 0;
        size_t cache_hits = 0;

        // For each point
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Set sentinel
            long pred = -1;

            ++cache_lookups;

            // Do we already have a prediction near this point?
            if (cache.check (p, i))
            {
                // Yes, use the previous prediction
                pred = cache.get_prediction (p, i);
                ++cache_hits;
            }
            else
            {
                // No, compute the prediction
                //
                // Create the raster at this point
                auto r = create_raster (p, i, sampling_params::patch_rows, sampling_params::patch_cols, sampling_params::aspect_ratio);

                // Create image Tensor from raster
                auto t = torch::from_blob (&r[0],
                    { 1, static_cast<int> (sampling_params::input_size) },
                    torch::kUInt8).to(torch::kFloat).to(device);

                // Decode
                auto prediction = network->forward (t).argmax(1);

                // Convert prediction from tensor to int
                pred = reverse_label_map.at (prediction[0].item<long> ());

                // Update the cache
                cache.update (p, i, pred);
            }

            // Check sentinel
            assert (pred != -1);

            // Save predicted value
            q[i].cls = pred;
        }

        // Keep track of performance
        unordered_map<long,confusion_matrix> cm;

        // Allocate confusion matrix for each classification
        cm[0] = confusion_matrix ();
        cm[40] = confusion_matrix ();
        cm[41] = confusion_matrix ();

        // Get results
        //
        // For each point
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Get actual value
            const long actual = p[i].cls;

            // Get predicted value
            const long pred = q[i].cls;

            for (auto j : cm)
            {
                // Get the key
                const auto cls = j.first;

                // Update the matrix
                const bool is_present = (actual == cls);
                const bool is_predicted = (pred == cls);
                cm[cls].update (is_present, is_predicted);
            }
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

        // Copy map so that it's ordered
        std::map<long,confusion_matrix> m (cm.begin (), cm.end ());
        for (auto i : m)
        {
            const auto key = i.first;
            ss << key
                << "\t" << cm[key].accuracy ()
                << "\t" << cm[key].F1 ()
                << "\t" << cm[key].balanced_accuracy ()
                << "\t" << cm[key].true_positives ()
                << "\t" << cm[key].true_negatives ()
                << "\t" << cm[key].false_positives ()
                << "\t" << cm[key].false_negatives ()
                << "\t" << cm[key].support ()
                << "\t" << cm[key].total ()
                << endl;
            if (!isnan (cm[key].F1 ()))
                weighted_f1 += cm[key].F1 () * cm[key].support () / cm[key].total ();
            if (!isnan (cm[key].accuracy ()))
                weighted_accuracy += cm[key].accuracy () * cm[key].support () / cm[key].total ();
            if (!isnan (cm[key].balanced_accuracy ()))
                weighted_bal_acc += cm[key].balanced_accuracy () * cm[key].support () / cm[key].total ();
        }
        ss << "weighted_F1 = " << weighted_f1 << endl;
        ss << "weighted_accuracy = " << weighted_accuracy << endl;
        ss << "weighted_bal_acc = " << weighted_bal_acc << endl;
        ss << "cache usage = " << 100.0 * cache_hits / cache_lookups << "%" << endl;

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
