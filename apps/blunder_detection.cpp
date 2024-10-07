#include "blunder_detection.h"
#include "blunder_detection_cmd.h"
#include "coastnet.h"
#include "dataframe.h"
#include "utils.h"

const std::string usage {"blunder_detection [options] < input.csv > output.csv"};

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_coastnet;

    try
    {
        // Parse the args
        auto args = cmd::get_args (argc, argv, usage);

        // Set defaults
        if (std::isnan (args.surface_min_elevation))
            args.surface_min_elevation = surface_min_elevation;
        if (std::isnan (args.surface_max_elevation))
            args.surface_max_elevation = surface_max_elevation;
        if (std::isnan (args.bathy_min_elevation))
            args.bathy_min_elevation = bathy_min_elevation;
        if (std::isnan (args.water_column_width))
            args.water_column_width = water_column_width;
        if (std::isnan (args.surface_range))
            args.surface_range = surface_range;
        if (std::isnan (args.bathy_range))
            args.bathy_range = bathy_range;

        // If you are getting help, exit without an error
        if (args.help)
            return 0;

        if (args.verbose)
        {
            // Show the args
            clog << "cmd_line_parameters:" << endl;
            clog << args;
        }

        // Read the points
        const auto df = ATL24_coastnet::dataframe::read (cin);

        // Convert it to the correct format
        bool has_manual_label = false;
        bool has_predictions = false;
        const auto p = convert_dataframe (df, has_manual_label, has_predictions);

        if (args.verbose)
            clog << p.size () << " points read" << endl;

        if (has_predictions == false)
            throw runtime_error ("Expected the dataframe to have predictions, but none were found");

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

        if (args.verbose)
            clog << "Getting surface and bathy estimates" << endl;

        // Compute surface and bathy estimates
        const auto s = get_surface_estimates (q, args.surface_sigma);
        const auto b = get_bathy_estimates (q, args.bathy_sigma);

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

        q = blunder_detection (q, args);

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

        // Write re-classified output to stdout
        write_classified_point2d (cout, q);

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
