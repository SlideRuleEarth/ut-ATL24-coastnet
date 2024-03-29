#include "ATL24_coastnet/blunder_detection.h"
#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_utils/dataframe.h"
#include "blunder_detection_cmd.h"

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
        const auto df = ATL24_utils::dataframe::read (cin);

        // Convert it to the correct format
        bool has_predictions = false;
        auto p = convert_dataframe (df, has_predictions);

        if (args.verbose)
            clog << p.size () << " points read" << endl;

        if (has_predictions == false)
            throw runtime_error ("Expected the dataframe to have predictions, but none were found");

        if (args.verbose)
            clog << "Sorting points" << endl;

        // Sort them by X
        sort (p.begin (), p.end (),
            [](const auto &a, const auto &b)
            { return a.x < b.x; });

        if (args.verbose)
            clog << "Re-classifying points" << endl;

        // Copy the photons
        auto q (p);

        const size_t iterations = 3;

        for (size_t i = 0; i < iterations; ++i)
        {
            if (args.verbose)
                clog << "Blunder detection pass " << i << endl;

            q = blunder_detection (p, args);

            // Check logic
            assert (p.size () == q.size ());

            // Rerun surface and bathy estimates
            const auto s = get_surface_estimates (q, args.surface_sigma);
            const auto b = get_bathy_estimates (q, args.bathy_sigma);

            assert (s.size () == q.size ());
            assert (b.size () == q.size ());

            for (size_t j = 0; j < q.size (); ++j)
            {
                q[j].surface_elevation = s[j];
                q[j].bathy_elevation = b[j];
            }
        }

        if (args.verbose)
        {
            size_t changed = 0;

            for (size_t i = 0; i < p.size (); ++i)
                if (p[i].prediction != q[i].prediction)
                    ++changed;
            clog << changed << " point classifications changed" << endl;
        }

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
