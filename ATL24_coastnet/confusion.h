#pragma once

#include "viper/raster.h"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace ATL24_coastnet
{

class confusion_matrix
{
    public:
    confusion_matrix ()
        : tp (0.0)
        , tn (0.0)
        , fp (0.0)
        , fn (0.0)
    {
    }

    // Matrix count access functions
    size_t true_positives () const { return static_cast<size_t> (tp); }
    size_t true_negatives () const { return static_cast<size_t> (tn); }
    size_t false_positives () const { return static_cast<size_t> (fp); }
    size_t false_negatives () const { return static_cast<size_t> (fn); }
    size_t support () const { return static_cast<size_t> (tp + fn); }
    size_t total () const { return static_cast<size_t> (tp + tn + fp + fn); }

    /// @brief Update the matrix
    /// @param present Truth value. True if class is present.
    /// @param prediction Prediction value. True if we predicted that the class is present.
    void update (bool present, bool prediction)
    {
        tp += ( present) && ( prediction); // hit
        tn += (!present) && (!prediction); // correct rejection
        fp += (!present) && ( prediction); // type I: false positive
        fn += ( present) && (!prediction); // type II: false negative
    }

    /// @brief Update the matrix
    /// @param m Add matrix counts to this matrix's counts
    void update (const confusion_matrix &m)
    {
        tp += m.tp;
        tn += m.tn;
        fp += m.fp;
        fn += m.fn;
    }

    double accuracy () const { return (tp + tn) / (tp + tn + fp + fn); }
    double precision () const { return positive_predictive_value (); }
    double recall () const { return true_positive_rate (); }
    double sensitivity () const { return true_positive_rate (); }
    double true_positive_rate () const { return tp / (tp + fn); }
    double specificity () const { return true_negative_rate (); }
    double true_negative_rate () const { return tn / (fp + tn); }
    double positive_predictive_value () const { return tp / (tp + fp); }
    double negative_predictive_value () const { return tn / (tn + fn); }
    double fallout () const { return false_positive_rate (); }
    double false_positive_rate () const { return fp / (fp + tn); }
    double false_discovery_rate () const { return fp / (fp + tp); }
    double miss_rate () const { return false_negative_rate (); }
    double false_negative_rate () const { return fn / (fn + tp); }

    // The harmonic mean of precision and sensitivity
    double F1 () const { return 2.0 * precision () * recall () / (precision () + recall ()); }

    // F_beta, with beta=2.0. This emphasizes false negatives.
    double F2 () const { return (1.0 + 2.0 * 2.0) * precision () * recall () / (2.0 * 2.0 * precision () + recall ()); }

    // F_beta, with beta=0.5. This attenuates false negatives.
    double F0_5 () const { return (1.0 + 0.5 * 0.5) * precision () * recall () / (0.5 * 0.5 * precision () + recall ()); }

    // Average of specificity and recall
    double balanced_accuracy () const { return (specificity () + recall ()) / 2.0; }

    // Calibrated F-score
    double calibrated_F_beta (const double r0 = 0.5, const double beta = 1.0) const
    {
        const double tpr = true_positive_rate ();
        const double fpr = false_positive_rate ();
        const double f1 = (1.0 + beta * beta) * tpr / (tpr + (1.0 / r0) * fpr + beta * beta);
        return f1;
    }

    // Matthews correlation coefficient
    //
    // The correlation coefficient between the observed and predicted classifications
    double MCC () const
    {
        const double x = (tp + fp) * (tp + fn) * (tn + fp) * (tn + fn);
        if (x > 0.0)
            return (tp * tn - fp * fn) / sqrt (x);
        else
            return 0.0;
    }

    friend std::ostream& operator<< (std::ostream &s, const confusion_matrix &cm)
    {
        s << "true_positives " << cm.true_positives () << std::endl;
        s << "true_negatives " << cm.true_negatives () << std::endl;
        s << "false_positives " << cm.false_positives () << std::endl;
        s << "false_negatives " << cm.false_negatives () << std::endl;
        s << "total " << cm.total () << std::endl;
        s << "support " << cm.support () << std::endl;
        return s;
    }

    private:
    double tp;
    double tn;
    double fp;
    double fn;
};

} // namespace ATL24_coastnet
