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
        // Check if assertions are enabled
        bool is_debug = false;
        assert (is_debug = true);
        clog << "Debug is " << (is_debug ? "ON" : "OFF") << endl;

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
            clog << "Re-classifying points" << endl;

        const auto q = blunder_detection (p, args);

        // Check logic
        assert (p.size () == q.size ());

        if (args.verbose)
        {
            size_t changed = 0;

            for (size_t i = 0; i < p.size (); ++i)
                if (p[i].cls != q[i].cls)
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
