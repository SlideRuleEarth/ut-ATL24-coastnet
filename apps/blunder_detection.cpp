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

        const auto q = blunder_detection (p);

        // Check logic
        assert (p.size () == q.size ());

        if (args.verbose)
        {
            size_t changed = 0;

            for (size_t i = 0; i < p.size (); ++i)
                if (p[i].cls != q[i])
                    ++changed;
            clog << changed << " point classifications changed" << endl;
        }

        // Write re-classified output to stdout
        write_classified_point2d (cout, p, q);

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
