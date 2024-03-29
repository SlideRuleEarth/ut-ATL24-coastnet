#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_utils/dataframe.h"
#include "combine_predictions_cmd.h"

const std::string usage {"combine_predictions [options] input1.csv input2.csv > output.csv"};

template<typename T>
T combine (const T &p1, const T &p2)
{
    using namespace std;

    if (p1.size () != p2.size ())
        throw runtime_error ("The dateframes do not have the same number of points");

    // Copy p1
    T q (p1);

    for (size_t i = 0; i < q.size (); ++i)
    {
        // Sanity check
        if (p1[i].h5_index != p2[i].h5_index)
            throw runtime_error ("The h5 indexes do not match in these two datasets");

        // Override only if the first one is not set
        if (q[i].prediction == 0)
            q[i].prediction = p2[i].prediction;
    }

    return q;
}

int main (int argc, char **argv)
{
    using namespace std;
    using namespace ATL24_coastnet;

    try
    {
        // Parse the args
        auto args = cmd::get_args (argc, argv, usage);

        // Read the dataframes
        if (args.verbose)
            clog << "Reading " << args.fn1 << endl;

        ifstream ifs1 (args.fn1);
        if (!ifs1)
            throw runtime_error (args.fn1 + ": Could not open file for reading");

        const auto df1 = ATL24_utils::dataframe::read (ifs1);

        if (args.verbose)
            clog << "Reading " << args.fn2 << endl;

        ifstream ifs2 (args.fn2);
        if (!ifs2)
            throw runtime_error (args.fn2 + ": Could not open file for reading");

        const auto df2 = ATL24_utils::dataframe::read (ifs2);

        // Convert it to the correct format
        bool has_predictions = false;
        auto p1 = convert_dataframe (df1, has_predictions);

        if (args.verbose)
            clog << p1.size () << " points read" << endl;

        if (has_predictions == false)
            throw runtime_error ("Expected the dataframe to have predictions, but none were found");

        auto p2 = convert_dataframe (df2, has_predictions);

        if (args.verbose)
            clog << p2.size () << " points read" << endl;

        if (has_predictions == false)
            throw runtime_error ("Expected the dataframe to have predictions, but none were found");

        // Merge them into one
        auto q = combine (p1, p2);

        const auto s = get_surface_estimates (q);
        const auto b = get_bathy_estimates (q);

        assert (s.size () == q.size ());
        assert (b.size () == q.size ());

        for (size_t i = 0; i < q.size (); ++i)
        {
            q[i].surface_elevation = s[i];
            q[i].bathy_elevation = b[i];
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
