#include "blunder_detection.h"
#include "cmd_utils.h"
#include "coastnet.h"
#include "dataframe.h"
#include "utils.h"
#include "classify_cmd.h"

const std::string usage {"classify [options] < filename.csv"};

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
            clog << "sampling parameters:" << endl;
            print_sampling_params (clog);
            clog << "Reading points from stdin" << endl;
        }

        // Read the points
        const auto df = ATL24_coastnet::dataframe::read (cin);

        // Convert it to the correct format
        bool has_manual_label;
        bool has_predictions;
        const auto p = convert_dataframe (df, has_manual_label, has_predictions);

        // Check args
        if (args.use_predictions && has_predictions == false)
            throw runtime_error ("'use-predictions' was specified, but the input file does not contain predictions");

        if (args.verbose)
            clog << p.size () << " points read" << endl;

        // Copy points
        auto q (p);

        // Get indexes into q
        vector<size_t> sorted_indexes (q.size ());

        // 0, 1, 2, ...
        iota (sorted_indexes.begin (), sorted_indexes.end (), 0);

        // Sort indexes by X
        sort (sorted_indexes.begin (), sorted_indexes.end (),
            [&](const auto &a, const auto &b)
            { return q[a].x < q[b].x; });

        // Sort points by X
        {
        auto tmp (q);
        for (size_t i = 0; i < sorted_indexes.size (); ++i)
            q[i] = tmp[sorted_indexes[i]];
        }

        // Classify them
        q = classify (args.verbose, q, args.model_filename, args.use_predictions);
        assert (q.size () == p.size ());

        if (args.verbose)
            clog << "Getting surface and bathy estimates" << endl;

        // Do post-processing
        postprocess_params params;

        // Compute surface and bathy estimates
        const auto s = get_surface_estimates (q, params.surface_sigma);
        const auto b = get_bathy_estimates (q, params.bathy_sigma);

        assert (s.size () == q.size ());
        assert (b.size () == q.size ());

        // Assign surface and bathy estimates
        for (size_t j = 0; j < q.size (); ++j)
        {
            q[j].surface_elevation = s[j];
            q[j].bathy_elevation = b[j];
        }

        if (args.verbose)
            clog << "Re-classifying points" << endl;

        q = blunder_detection (q, params);

        // Restore original order
        {
        auto tmp (q);
        for (size_t i = 0; i < sorted_indexes.size (); ++i)
            q[sorted_indexes[i]] = tmp[i];
        }

        // Ensure photon order did not change
#pragma omp parallel for
        for ([[maybe_unused]] size_t i = 0; i < q.size (); ++i)
            assert (p[i].h5_index == q[i].h5_index);

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
