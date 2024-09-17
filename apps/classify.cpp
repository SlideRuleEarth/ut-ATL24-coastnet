#include "classify_cmd.h"
#include "coastnet.h"
#include "dataframe.h"
#include "utils.h"

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
            clog << endl;
            clog << "Reading points from stdin" << endl;
            clog << endl;
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

        // Classify them
        const auto q = classify (p, args);
        assert (q.size () == p.size ());

        // Ensure photon order did not change
#pragma omp parallel for
        for (size_t i = 0; i < q.size (); ++i)
        {
            assert (p[i].h5_index == q[i].h5_index);
            ((void) (i)); // Eliminate unused variable warning
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
