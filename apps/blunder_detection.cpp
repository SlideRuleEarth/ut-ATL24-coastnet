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
        auto p = convert_dataframe (df, has_manual_label, has_predictions);

        if (args.verbose)
            clog << p.size () << " points read" << endl;

        if (has_predictions == false)
            throw runtime_error ("Expected the dataframe to have predictions, but none were found");

        // Get indexes into p
        vector<size_t> sorted_indexes (p.size ());

        // 0, 1, 2, ...
        iota (sorted_indexes.begin (), sorted_indexes.end (), 0);

        // Sort indexes by X
        sort (sorted_indexes.begin (), sorted_indexes.end (),
            [&](const auto &a, const auto &b)
            { return p[a].x < p[b].x; });

        // Sort points by X
        {
        auto tmp (p);
        for (size_t i = 0; i < sorted_indexes.size (); ++i)
            p[i] = tmp[sorted_indexes[i]];
        }

        if (args.verbose)
            clog << "Getting surface and bathy estimates" << endl;

        // Compute surface and bathy estimates
        const auto s = get_surface_estimates (p, args.surface_sigma);
        const auto b = get_bathy_estimates (p, args.bathy_sigma);

        assert (s.size () == p.size ());
        assert (b.size () == p.size ());

        // Assign surface and bathy estimates
        for (size_t j = 0; j < p.size (); ++j)
        {
            p[j].surface_elevation = s[j];
            p[j].bathy_elevation = b[j];
        }

        if (args.verbose)
            clog << "Re-classifying points" << endl;

        auto q = blunder_detection (p, args);

        // Check logic
        assert (p.size () == q.size ());

        if (args.verbose)
        {
            size_t changed = 0;

            for (size_t i = 0; i < p.size (); ++i)
                if (p[i].prediction != q[i].prediction)
                    ++changed;
            clog << changed << " point classifications changed" << endl;
        }

        // Restore original order
        {
        auto tmp (q);
        for (size_t i = 0; i < sorted_indexes.size (); ++i)
            q[sorted_indexes[i]] = tmp[i];
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
