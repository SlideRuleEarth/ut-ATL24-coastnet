#include "ATL24_coastnet/coastnet.h"
#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_utils/dataframe.h"
#include "viper/raster.h"
#include "coastnet_network.h"
#include "classify_coastnet_cmd.h"

const std::string usage {"ls *.csv | classify_coastnet [options]"};

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_coastnet;

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
            clog << args;
        }

        // Params
        sampling_params sp;
        hyper_params hp;

        // Override aspect-ratio if specified
        if (args.aspect_ratio != 0)
            sp.aspect_ratio = args.aspect_ratio;

        if (args.verbose)
        {
            clog << "sampling parameters:" << endl;
            clog << sp << endl;
            clog << "hyper parameters:" << endl;
            clog << hp << endl;
        }

        // Create the network
        const size_t num_classes = 7;
        std::array<int64_t, 3> layers{2, 2, 2};
        ResNet<ResidualBlock> network (layers, num_classes);

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
        torch::Device device (torch::kCUDA);

        // Prepare for inference
        network->train (false);
        network->to (device);

        // Disabling gradient calculation is useful for inference,
        // when you are sure that you will not call Tensor::backward.
        // It will reduce memory consumption and speed up computations
        // that would otherwise have requires_grad() == true.
        torch::NoGradGuard no_grad;

        // Read the points
        const auto df = ATL24_utils::dataframe::read (cin);

        // Convert it to the correct format
        bool has_predictions = false;
        bool has_sea_surface = false;
        auto p = convert_dataframe (df, has_predictions, has_sea_surface);

        if (args.verbose)
        {
            clog << p.size () << " points read" << endl;
            if (has_predictions)
                clog << "Dataframe contains predictions" << endl;
            if (has_sea_surface)
                clog << "Dataframe contains sea surface estimates" << endl;
            clog << "Sorting points" << endl;
        }

        // Sort them by X
        sort (p.begin (), p.end (),
            [](const auto &a, const auto &b)
            { return a.x < b.x; });

        if (args.verbose)
            clog << "Classifying points" << endl;

        // Allocate space for predictions
        vector<size_t> q (p.size ());

        // Setup the prediction cache
        prediction_cache cache;

        // Keep track of cache usage
        size_t cache_lookups = 0;
        size_t cache_hits = 0;

        // For each point
#pragma omp parallel for
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Set sentinel
            long pred = -1;

#pragma omp atomic
            ++cache_lookups;

            // Don't override non-noise predictions
            if (has_predictions && p[i].prediction != 0)
            {
                q[i] = p[i].prediction;
                continue;
            }

            // Do we already have a prediction near this point?
            if (cache.check (p, i))
            {
                // Yes, use the previous prediction
                pred = cache.get_prediction (p, i);
#pragma omp atomic
                ++cache_hits;
            }
            else
            {
                // No, compute the prediction
                //
                // Are we predicting water surface?
                if ((args.cls == 41) &&
                    (p[i].z < surface_min_elevation || p[i].z > surface_max_elevation))
                {
                    // Can't be water surface
                    pred = 0;
                }
                else
                {
                    // Create the raster at this point
                    auto r = create_raster (p, i, sp.patch_rows, sp.patch_cols, sp.aspect_ratio);

                    // Create image Tensor from raster
                    auto t = torch::from_blob (&r[0],
                        { static_cast<int> (sp.patch_rows), static_cast<int> (sp.patch_cols) },
                        torch::kUInt8).to(torch::kFloat);

                    // [rows cols] -> [1 1 rows cols]
                    t = t.unsqueeze(0).unsqueeze(0).to(device);

                    // Decode
                    const auto predictions = network->forward (t);
                    const auto prediction = predictions.argmax(1);

                    // Convert prediction from tensor to int
                    pred = reverse_label_map.at (prediction[0].item<long> ());
                }

                // Update the cache
#pragma omp critical
                cache.update (p, i, pred);
            }

            // Check sentinel
            assert (pred != -1);

            // Save predicted value
            q[i] = pred;
        }

        if (args.verbose)
            clog << "cache usage = " << 100.0 * cache_hits / cache_lookups << "%" << endl;

        // Assign predictions
        assert (p.size () == q.size ());
        for (size_t i = 0; i < p.size (); ++i)
            p[i].prediction = q[i];

        // Get surface estimates
        const auto s = get_surface_estimates (p);

        // Assign surface estimates
        assert (p.size () == s.size ());
        for (size_t i = 0; i < p.size (); ++i)
            p[i].sea_surface = s[i];

        // Write classified output to stdout
        write_classified_point2d (cout, p);

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
