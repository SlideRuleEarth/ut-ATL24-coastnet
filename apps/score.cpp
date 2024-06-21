#include "ATL24_coastnet/coastnet.h"
#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/dataframe.h"
#include "score_cmd.h"

const std::string usage {"score_coastnet < filename.csv"};

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
            // Show the args
            clog << "cmd_line_parameters:" << endl;
            clog << args;
            clog << "Reading points from stdin" << endl;
        }

        // Read the points
        const auto df = ATL24_coastnet::dataframe::read (cin);

        // Convert it to the correct format
        bool has_manual_label = false;
        bool has_predictions = false;
        const auto p = convert_dataframe (df, has_manual_label, has_predictions);

        if (args.verbose)
        {
            clog << p.size () << " points read" << endl;
            if (has_manual_label)
                clog << "Dataframe contains manual labels" << endl;
            if (has_predictions)
                clog << "Dataframe contains predictions" << endl;
            clog << "Sorting points" << endl;
        }

        // Do this for one or more classes
        set<long> classes;

        if (args.cls != -1)
        {
            // Do it for the specified class
            classes.insert (args.cls);
        }
        else
        {
            // Do it for all classes in the dataset
            for (size_t i = 0; i < p.size (); ++i)
                classes.insert (p[i].cls);
        }

        if (args.verbose)
        {
            clog << "Computing scores for:";
            for (auto c : classes)
                clog << " " << c;
            clog << endl;
        }

        // Keep track of performance
        unordered_map<long,confusion_matrix> cm;

        // Allocate cm
        cm[0] = confusion_matrix ();

        // For each classification
        for (auto cls : classes)
        {
            // Allocate cm
            cm[cls] = confusion_matrix ();

            // For each point
            for (size_t i = 0; i < p.size (); ++i)
            {
                // Get values
                const long actual = static_cast<long> (p[i].cls);
                const long pred = static_cast<int> (p[i].prediction);

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
            << "\t" << "cal_F1"
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
        double weighted_cal_f1 = 0.0;

        // Copy map so that it's ordered
        std::map<long,confusion_matrix> m (cm.begin (), cm.end ());
        for (auto i : m)
        {
            const auto key = i.first;
            ss << key
                << "\t" << cm[key].accuracy ()
                << "\t" << cm[key].F1 ()
                << "\t" << cm[key].balanced_accuracy ()
                << "\t" << cm[key].calibrated_F_beta ()
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
            if (!isnan (cm[key].calibrated_F_beta ()))
                weighted_cal_f1 += cm[key].calibrated_F_beta () * cm[key].support () / cm[key].total ();
        }
        ss << "weighted_accuracy = " << weighted_accuracy << endl;
        ss << "weighted_F1 = " << weighted_f1 << endl;
        ss << "weighted_bal_acc = " << weighted_bal_acc << endl;
        ss << "weighted_cal_F1 = " << weighted_cal_f1 << endl;

        // Show results
        if (args.verbose)
            clog << ss.str ();

        // Write results to stdout
        cout << ss.str ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
