#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/dataframe.h"
#include "ATL24_coastnet/raster.h"
#include "features.h"
#include "xgboost.h"
#include "classify_cmd.h"

const std::string usage {"ls *.csv | resnet [options]"};

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
        }

        if (args.verbose)
        {
            clog << "sampling parameters:" << endl;
            print_sampling_params (clog);
            clog << endl;
        }

        // Create the booster
        xgboost::xgbooster xgb (args.verbose);
        xgb.load_model (args.model_filename);

        // Read the points
        const auto df = ATL24_coastnet::dataframe::read (cin);

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
            clog << "Classifying points" << endl;

        // Copy the points
        auto q (p);

        // Predict in batches
        const size_t batch_size = 1000;

        // For each point
        for (size_t i = 0; i < p.size (); i += batch_size)
        {
            // Get number of samples to predict
            const size_t rows = (i + batch_size < p.size ())
                ? batch_size
                : p.size () - i;

            // Create the features
            const size_t cols = features_per_sample ();
            vector<float> f (rows * cols);

            // Get the rasters for each point
            for (size_t j = 0; j < rows; ++j)
            {
                // Get point sample index
                const size_t index = i + j;
                assert (index < p.size ());

                // Create the raster at this point
                auto r = create_raster (p, index, sampling_params::patch_rows, sampling_params::patch_cols, sampling_params::aspect_ratio);

                // The first feature is the elevation
                f[j * cols] = p[index].z;

                // The rest of the features are the raster values
                for (size_t k = 0; k < r.size (); ++k)
                {
                    const size_t n = j * cols + 1 + k;
                    assert (n < f.size ());
                    f[n] = r[k];
                }
            }

            // Process the batch
            const auto predictions = xgb.predict (f, rows, cols);
            assert (predictions.size () == rows);

            // Get the prediction for each batch point
            for (size_t j = 0; j < rows; ++j)
            {
                // Remap prediction
                const unsigned pred = reverse_label_map.at (predictions[j]);

                // Get point sample index
                const size_t index = i + j;
                assert (index < q.size ());

                // Save predicted value
                q[index].cls = pred;
            }
        }

        // Keep track of performance
        unordered_map<long,confusion_matrix> cm;

        // Allocate confusion matrix for each classification
        cm[0] = confusion_matrix ();
        cm[40] = confusion_matrix ();
        cm[41] = confusion_matrix ();

        // Get results
        //
        // For each point
        for (size_t i = 0; i < p.size (); ++i)
        {
            // Get actual value
            const long actual = p[i].cls;

            // Get predicted value
            const long pred = q[i].cls;

            for (auto j : cm)
            {
                // Get the key
                const auto cls = j.first;

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

        // Write results, if specified
        if (!args.results_filename.empty ())
        {
            if (args.verbose)
                clog << "Writing " << args.results_filename << endl;

            ofstream ofs (args.results_filename);

            if (!ofs)
                cerr << "Could not open file for writing" << endl;
            else
                ofs << ss.str ();
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
