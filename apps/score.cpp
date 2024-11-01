#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/dataframe.h"
#include "score_cmd.h"
#include "ATL24_coastnet/coastnet.h"

using namespace std;
using namespace ATL24_coastnet;

const string usage {"score < filename.csv"};

string get_confusion_matrix_header ()
{
    stringstream ss;
    ss << "cls"
        << "\t" << "acc"
        << "\t" << "F1"
        << "\t" << "bal_acc"
        << "\t" << "cal_F1"
        << "\t" << "MCC"
        << "\t" << "Avg"
        << "\t" << "tp"
        << "\t" << "tn"
        << "\t" << "fp"
        << "\t" << "fn"
        << "\t" << "support"
        << "\t" << "total";
    return ss.str ();
}

string print (const long cls, const confusion_matrix &cm)
{
    stringstream ss;
    ss << setprecision(3) << fixed;
    ss << cls
        << "\t" << cm.accuracy ()
        << "\t" << cm.F1 ()
        << "\t" << cm.balanced_accuracy ()
        << "\t" << cm.calibrated_F_beta ()
        << "\t" << cm.MCC ()
        << "\t" << (cm.F1 ()
                    + cm.balanced_accuracy ()
                    + cm.calibrated_F_beta ()
                    + cm.MCC ()) / 4.0
        << "\t" << cm.true_positives ()
        << "\t" << cm.true_negatives ()
        << "\t" << cm.false_positives ()
        << "\t" << cm.false_negatives ()
        << "\t" << cm.support ()
        << "\t" << cm.total ();

    return ss.str ();
}

unordered_map<long,confusion_matrix> get_confusion_matrix_map (
    const bool verbose,
    istream &is,
    const string &prediction_label,
    const long cls,
    const long ignore_cls)
{
    // Read the points
    const auto df = dataframe::read (is);

    if (verbose)
        clog << "Converting dataframe" << endl;

    // Convert it to the correct format
    const auto p = convert_dataframe (df);

    if (verbose)
        clog << p.size () << " points read" << endl;

    set<unsigned> classes;

    if (cls != -1)
        classes.insert (cls);
    else
    {
        classes.insert (0);
        classes.insert (40);
        classes.insert (41);
    }

    if (verbose)
    {
        clog << "Scoring points" << endl;
        clog << "Computing scores for:";
        for (auto c : classes)
            clog << " " << c;
        clog << endl;
    }

    // Keep track of performance
    unordered_map<long,confusion_matrix> cm;

    // Allocate cms
    for (auto i : classes)
        cm[i] = confusion_matrix ();

    size_t ignored = 0;

    // For each classification
    for (auto c : classes)
    {
        // Allocate cm
        cm[c] = confusion_matrix ();

        // For each point
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Get values
            long actual = static_cast<long> (p[i].cls);
            long pred = static_cast<int> (p[i].prediction);

            // Ignore it?
            if (actual == ignore_cls)
            {
                ++ignored;
                continue;
            }

            // Map 1 -> 0
            actual = actual == 1 ? 0 : actual;
            pred = pred == 1 ? 0 : pred;

            // Update the matrix
            const bool is_present = (actual == c);
            const bool is_predicted = (pred == c);
            cm[c].update (is_present, is_predicted);
        }
    }

    if (verbose)
        clog << "Ignored " << ignored << " points" << endl;

    return cm;
}

unordered_map<long,confusion_matrix> get_confusion_matrix_map (
    const bool verbose,
    const vector<string> &filenames,
    const string &prediction_label,
    const string &csv_filename,
    const long cls,
    const long ignore_cls)
{
    if (filenames.empty ())
    {
        clog << "No filenames specified. Reading dataframe from stdin..." << endl;
        return get_confusion_matrix_map (verbose, cin, prediction_label, cls, ignore_cls);
    }

    vector<unordered_map<long,confusion_matrix>> maps (filenames.size ());

    ofstream ofs;
    if (!csv_filename.empty ())
    {
        if (verbose)
            clog << "Writing CSV data to " << csv_filename << endl;

        ofs.open (csv_filename);

        if (!ofs)
            throw runtime_error ("Could not open file for writing");

        ofs << get_confusion_matrix_header ()
            << "\tmodel"
            << "\tfilename"
            << endl;
    }

#pragma omp parallel for
    for (size_t i = 0; i < filenames.size (); ++i)
    {
        if (verbose)
        {
#pragma omp critical
            clog << "Reading " << filenames[i] << endl;
        }

        ifstream ifs (filenames[i]);

        if (!ifs)
            throw runtime_error ("Could not open file for reading");

        maps[i] = get_confusion_matrix_map (verbose, ifs, prediction_label, cls, ignore_cls);

        if (ofs)
        {
            // Copy to map so that it's ordered
            map<long,confusion_matrix> tmp (maps[i].begin (), maps[i].end ());
            for (auto j : tmp)
                ofs << print (j.first, j.second)
                    << "\t" << (prediction_label.empty () ? "coastnet" : prediction_label)
                    << "\t" << filenames[i]
                    << endl;
        }
    }

    // Combine them all into one
    unordered_map<long,confusion_matrix> m;

    for (auto i : maps)
    {
        for (auto j : i)
        {
            const auto key = j.first;
            const auto cm = j.second;
            m[key].add (cm);
        }
    }

    return m;
}

int main (int argc, char **argv)
{
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
        }

        const auto tmp = get_confusion_matrix_map (
            args.verbose,
            args.filenames,
            args.prediction_label,
            args.csv_filename,
            args.cls,
            args.ignore_cls);

        // Copy map so that it's ordered
        map<long,confusion_matrix> cmm (tmp.begin (), tmp.end ());

        // Compile results
        stringstream ss;
        ss << get_confusion_matrix_header () << endl;

        for (auto i : cmm)
        {
            const auto key = i.first;
            const auto cm = i.second;
            ss << print (key, cm) << endl;
        }

        // If you're doing a multi-class score, computed weighted scores too
        if (args.cls == -1)
        {
            double weighted_f1 = 0.0;
            double weighted_accuracy = 0.0;
            double weighted_bal_acc = 0.0;
            double weighted_cal_f1 = 0.0;
            double weighted_MCC = 0.0;
            for (auto i : cmm)
            {
                const auto cm = i.second;
                if (!isnan (cm.F1 ()))
                    weighted_f1 += cm.F1 () * cm.support () / cm.total ();
                if (!isnan (cm.accuracy ()))
                    weighted_accuracy += cm.accuracy () * cm.support () / cm.total ();
                if (!isnan (cm.balanced_accuracy ()))
                    weighted_bal_acc += cm.balanced_accuracy () * cm.support () / cm.total ();
                if (!isnan (cm.calibrated_F_beta ()))
                    weighted_cal_f1 += cm.calibrated_F_beta () * cm.support () / cm.total ();
                if (!isnan (cm.MCC ()))
                    weighted_MCC += cm.MCC () * cm.support () / cm.total ();
            }
            ss << "weighted_accuracy = " << weighted_accuracy << endl;
            ss << "weighted_F1 = " << weighted_f1 << endl;
            ss << "weighted_bal_acc = " << weighted_bal_acc << endl;
            ss << "weighted_cal_F1 = " << weighted_cal_f1 << endl;
            ss << "weighted_MCC = " << weighted_MCC << endl;
        }

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
