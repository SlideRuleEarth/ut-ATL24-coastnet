#pragma once

#include "precompiled.h"
#include "confusion.h"
#include "xgboost.h"

namespace ATL24_coastnet
{

constexpr double surface_min_elevation = -20.0; // meters
constexpr double surface_max_elevation = 20.0; // meters
constexpr double bathy_min_elevation = -100.0; // meters
constexpr double water_column_width = 100.0; // meters
constexpr double surface_range = 3.0; // +-meters
constexpr double bathy_range = 3.0; // +-meters

// ASPRS Definitions
constexpr unsigned unclassified_class = 1;
constexpr unsigned bathy_class = 40;
constexpr unsigned sea_surface_class = 41;
constexpr unsigned water_column_class = 45;

// The classifier wants the labels to be 0-based and sequential,
// so remap the ASPRS labels during data loading
std::unordered_map<long,long> label_map = {
    {0, 0}, // unlabeled
    {7, 0}, // noise
    {2, 1}, // ground
    {4, 2}, // vegetation
    {5, 3}, // canopy
    {41, 4}, // sea surface
    {45, 5}, // water column
    {40, 6}, // bathymetry
};

std::unordered_map<long,long> reverse_label_map = {
    {0, 0},
    {1, 2},
    {2, 4},
    {3, 5},
    {4, 41},
    {5, 45},
    {6, 40},
};

struct point2d
{
    size_t h5_index;
    double x, z;
};

struct point3d
{
    size_t h5_index;
    double ts, lat, lon, z;
};

struct classified_point2d
{
    size_t h5_index;
    double x;
    double z;
    size_t cls;
    size_t prediction;
    double surface_elevation;
    double bathy_elevation;
};

std::ostream& operator<< (std::ostream &s, const classified_point2d &p)
{
    s << std::setprecision(15) << std::fixed;
    s << "h5_index\t" << p.h5_index << std::endl;
    s << "x\t" << p.x << std::endl;
    s << "z\t" << p.z << std::endl;
    s << "cls\t" << p.cls << std::endl;
    s << "prediction\t" << p.prediction << std::endl;
    s << "surface_elevation\t" << p.surface_elevation << std::endl;
    s << "bathy_elevation\t" << p.bathy_elevation << std::endl;
    return s;
}

struct classified_point3d
{
    size_t h5_index;
    double ts, lat, lon, z;
    size_t cls;
};

template<typename T>
std::vector<size_t> get_nearest_along_track_prediction (const T &p, const unsigned c)
{
    // At each point in 'p', what is the index of the closest point
    // with the label 'c'?
    using namespace std;

    // Set sentinels
    vector<size_t> indexes (p.size (), p.size ());

    // Check data
    if (p.empty ())
        return indexes;

    // Get first and last indexes with label 'c'
    size_t first_index = p.size ();
    size_t last_index = p.size ();
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore ones that are the wrong class
        if (p[i].prediction != c)
            continue;

        // Only set it if it's the first
        if (first_index == p.size ())
            first_index = i;

        // Always set the last
        last_index = i;
    }

    // If we didn't find at least one, there is nothing to do
    if (first_index == p.size ())
        return indexes;

    // Check logic
    assert (last_index != p.size ());

    // Set all of the indexes to the left of 'first_index'
    for (size_t i = 0; i < first_index; ++i)
        indexes[i] = first_index;

    // Set all of the indexes to the right of 'last_index'
    for (size_t i = last_index; i < p.size (); ++i)
        indexes[i] = last_index;

    // Set sentinels
    size_t left_index = p.size ();
    size_t right_index = p.size ();

    // Now set all of the indexes between 'first_index' and
    // 'last_index'
    for (size_t i = first_index; i < last_index; ++i)
    {
        // Is this a label that we are interested in?
        if (p[i].prediction == c)
        {
            // Closest point with label 'c' is itself
            indexes[i] = i;

            // Save its position
            right_index = left_index = i;

            continue;
        }

        // Search to the right for the next index with label 'c'
        if (right_index < i)
        {
            for (size_t j = i; j <= last_index; ++j)
            {
                if (p[j].prediction == c)
                {
                    right_index = j;
                    break;
                }
            }
        }

        // Logic check
        assert (left_index < i);
        assert (i < right_index);
        assert (p[left_index].x <= p[i].x);
        assert (p[i].x <= p[right_index].x);

        // Set the index of the closer of the two
        const double d_left = p[i].x - p[left_index].x;
        const double d_right = p[right_index].x - p[i].x;

        if (d_left <= d_right)
            indexes[i] = left_index;
        else
            indexes[i] = right_index;
    }

    // Logic check
    for (size_t i = 0; i < indexes.size (); ++i)
        assert (indexes[i] < p.size ());

    return indexes;
}

template<typename T>
size_t count_predictions (const T &p, const unsigned cls)
{
    return std::count_if (p.begin (), p.end (), [&](auto i)
    { return i.prediction == cls; });
}

template<typename T>
std::vector<double> get_quantized_average (const T &p, const unsigned cls)
{
    using namespace std;

    assert (!p.empty ());

    // Get extent
    const unsigned min_x = p[0].x;
    const unsigned max_x = p.back ().x + 1.0;

    // Values should be sorted
    assert (min_x < max_x);
    const size_t total = max_x - min_x;

    // Get 1m window averages
    vector<double> sums (total);
    vector<double> totals (total);
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Skip non-'cls' photons
        if (p[i].prediction != cls)
            continue;

        // Get along-track index
        const unsigned j = p[i].x - min_x;

        // Check logic
        assert (j < totals.size ());
        assert (j < sums.size ());

        // Count the value
        ++totals[j];
        sums[j] += p[i].z;
    }

    // Get the average
    vector<double> avg (total, NAN);
    for (size_t i = 0; i < avg.size (); ++i)
        if (totals[i] != 0)
            avg[i] = sums[i] / totals[i];

    return avg;
}

template<typename T>
std::vector<std::pair<size_t,size_t>> get_nan_pairs (const T &p)
{
    using namespace std;

    // For each block of consecutive nan's, get the indexes of the two
    // non-nan values on either side of the block
    vector<pair<size_t,size_t>> np;

    // Special case for first value
    if (std::isnan (p[0]))
        np.push_back (make_pair (0, 0));

    // Get the left side of each block
    for (size_t i = 0; i + 1 < p.size (); ++i)
    {
        if (!std::isnan (p[i]) && isnan (p[i + 1]))
            np.push_back (make_pair (i, i));
    }

    // Find the right side of each block
    for (size_t i = 0; i < np.size (); ++i)
    {
        for (size_t j = np[i].first + 1; j < p.size (); ++j)
        {
            // Skip over nans
            if (std::isnan (p[j]))
                continue;

            // Set the index of the right side
            np[i].second = j;
            break;
        }
    }

    // Special case for last value
    if (std::isnan (p.back ()))
        np.back ().second = p.size () - 1;

    // Check our logic
    for (size_t i = 0; i < np.size (); ++i)
        assert (np[i].first < np[i].second);

    return np;
}

template<typename T,typename U>
void interpolate_nans (T &p, const U &n)
{
    using namespace std;

    // Check logic
    assert (n.first < n.second);

    // Get values to interpolate between
    double left = p[n.first];
    double right = p[n.second];

    // Check special cases
    if (std::isnan (left))
    {
        // First value is series was a NAN
        assert (n.first == 0);
        assert (!std::isnan (right));
        left = right;
        p[0] = right;
    }
    if (std::isnan (right))
    {
        // Last value in series was a nan
        assert (n.second == p.size () - 1);
        assert (!std::isnan (left));
        right = left;
        p[p.size () - 1] = left;
    }

    const double len = n.second - n.first;
    assert (len > 0.0);
    for (size_t i = n.first + 1; i < n.second; ++i)
    {
        // Distance from left
        const double d = i - n.first;
        assert (d > 0.0);
        assert (d < len);
        // Get weight from right
        const double w = d / len;
        assert (w > 0.0);
        assert (w < 1.0);
        // Interpolate
        const double avg = (1.0 - w) * left + w * right;
        assert (i < p.size ());
        p[i] = avg;
    }
}

template<typename T>
T box_filter (const T &p, const int filter_width)
{
    // Check invariants
    assert ((filter_width & 1) != 0); // Must be an odd kernel
    assert (filter_width >= 3); // w=1 does not make sense

    // Keep cumulative sums and totals across the vector
    std::vector<double> sums (p.size ());
    std::vector<size_t> totals (p.size ());
    double cumulative_sum = 0.0;
    size_t cumulative_total = 0;

    // For each pixel get cumulative sums and counts
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Get running sum and total
        cumulative_sum += p[i];
        ++cumulative_total;

        // Remember them
        sums[i] = cumulative_sum;
        totals[i] = cumulative_total;
    }

    // Return value
    T q (p.size ());

    // Now go back and fill in filter values based upon sums and totals
    for (size_t i = 0; i < q.size (); ++i)
    {
        const int i1 = i - filter_width / 2 - 1;
        const int i2 = i + filter_width / 2;

        // Clip the index values to the edges of the row
        const double sum1 = (i1 < 0) ? 0 : sums[i1];
        const size_t total1 = (i1 < 0) ? 0 : totals[i1];
        const double sum2 = (i2 >= static_cast<int>(p.size ())) ? sums[p.size () - 1] : sums[i2];
        const size_t total2 = (i2 >= static_cast<int>(p.size ())) ? totals[p.size () - 1] : totals[i2];

        // Compute sum and total at pixel 'i'.
        const double sum = sum2 - sum1;
        const int total = total2 - total1;
        assert (total > 0);

        // Get the average over the window of size 'filter_width' at pixel 'i'
        q[i] = sum / total;
    }

    return q;
}

template<typename T>
std::vector<double> get_elevation_estimates (const T &p, const double sigma, const unsigned cls)
{
    using namespace std;

    // Return value
    vector<double> z (p.size (), numeric_limits<double>::max ());

    // Count number of 'cls' predictions
    const size_t total = count_predictions (p, cls);

    // Degenerate case
    if (total == 0)
        return z;

    // Get 1m window averages
    auto avg = get_quantized_average (p, cls);

    // Get indexes of values to interpolate between
    const auto np = get_nan_pairs (avg);

    // Interpolate values between each pair
    for (auto n : np)
        interpolate_nans (avg, n);

    // Run a box filter over the averaged values
    const unsigned iterations = 4;

    // See: https://www.peterkovesi.com/papers/FastGaussianSmoothing.pdf
    const double ideal_filter_width = std::sqrt ((12.0 * sigma * sigma) / iterations + 1.0);
    const int filter_width = std::max (static_cast<int> (ideal_filter_width / 2.0), 1) * 2 + 1;

    // Apply Gaussian smoothing
    for (size_t i = 0; i < iterations; ++i)
        avg = box_filter (avg, filter_width);

    // Get min extent
    const unsigned min_x = p[0].x;

    // Fill in the estimates with the filtered points
    for (size_t i = 0; i < z.size (); ++i)
    {
        // Values should be sorted
        assert (i < p.size ());
        assert (min_x <= p[i].x);

        // Get along-track index
        const unsigned j = p[i].x - min_x;

        // Count the value
        assert (i < z.size ());
        assert (j < avg.size ());
        z[i] = avg[j];
    }

    return z;
}

template<typename T>
std::vector<double> get_surface_estimates (const T &p, const double sigma)
{
    return get_elevation_estimates (p, sigma, sea_surface_class);
}

template<typename T>
std::vector<double> get_bathy_estimates (const T &p, const double sigma)
{
    return get_elevation_estimates (p, sigma, bathy_class);
}

namespace sampling_params
{
    const size_t patch_rows = 63;
    const size_t patch_cols = 15;
    const int64_t input_size = patch_rows * patch_cols;
    const double aspect_ratio = 4.0;
}

void print_sampling_params (std::ostream &os)
{
    os << "patch_rows: " << sampling_params::patch_rows << std::endl;
    os << "patch_cols: " << sampling_params::patch_cols << std::endl;
    os << "input_size: " << sampling_params::input_size << std::endl;
    os << "aspect_ratio: " << sampling_params::aspect_ratio << std::endl;
}

// total features =
//        photon elevation
//      + raster size
constexpr size_t FEATURES_PER_SAMPLE = 1 + sampling_params::patch_rows * sampling_params::patch_cols;

template<typename T,typename U>
std::vector<ATL24_coastnet::classified_point2d> classify (T p, const U &args)
{
    using namespace std;
    using namespace ATL24_coastnet;

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

    // Save the predictions
    vector<unsigned> q (p.size ());

    for (size_t i = 0; i < p.size (); ++i)
    {
        q[i] = p[i].prediction;
        p[i].prediction = 0;
    }

    // Create the booster
    xgboost::xgbooster xgb (args.verbose);
    xgb.load_model (args.model_filename);

    // Keep track of how many we skipped
    size_t used_predictions = 0;

    // Predict in batches
    const size_t batch_size = 1000;

    // For each point
    for (size_t i = 0; i < p.size (); )
    {
        // Get a vector of indexes
        vector<size_t> indexes;
        indexes.reserve (batch_size);

        // Fill the vector of indexes
        while (i < p.size () && indexes.size () != batch_size)
        {
            // Can we use the input predictions?
            if (args.use_predictions && p[i].prediction != 0)
            {
                // Use the prediction
                p[i].prediction = q[i];
                ++used_predictions;
            }
            else
            {
                // Save this index
                indexes.push_back (i);
            }

            // Go to the next one
            ++i;
        }

        // Get number of samples to predict
        const size_t rows = indexes.size ();

        // Create the features
        const size_t cols = FEATURES_PER_SAMPLE;
        vector<float> f (rows * cols);

        // Get the rasters for each point
        for (size_t j = 0; j < indexes.size (); ++j)
        {
            // Get point sample index
            assert (j < indexes.size ());
            const size_t index = indexes[j];
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
        for (size_t j = 0; j < indexes.size (); ++j)
        {
            // Remap prediction
            const unsigned pred = reverse_label_map.at (predictions[j]);

            // Get point sample index
            assert (j < indexes.size ());
            const size_t index = indexes[j];
            assert (index < p.size ());

            // Save predicted value
            p[index].prediction = pred;
        }
    }

    if (args.verbose)
        clog << "used predictions = " << 100.0 * used_predictions / p.size () << "%" << endl;

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
        const long pred = p[i].prediction;

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

    // Restore original order
    {
    auto tmp (p);
    for (size_t i = 0; i < sorted_indexes.size (); ++i)
        p[sorted_indexes[i]] = tmp[i];
    }

    return p;
}

template<typename T>
class features
{
    public:
    explicit features (const T &_dataset)
        : dataset (_dataset)
    {
    }
    size_t size () const
    {
        return dataset.size ();
    }
    std::vector<float> get_features () const
    {
        using namespace std;

        // Get all the features for this dataset
        const size_t rows = dataset.size ();
        const size_t cols = FEATURES_PER_SAMPLE;
        vector<float> f (rows * cols);

        // Stuff values into features vector
        for (size_t i = 0; i < rows; ++i)
        {
            // First feature is the photon's elevation
            assert (i * cols < f.size ());
            f[i * cols] = dataset.get_elevation (i);

            // The other features are the raster values
            const auto r = dataset.get_raster (i);

            for (size_t j = 0; j < r.size (); ++j)
            {
                const size_t index = i * cols + 1 + j;
                assert (index < f.size ());
                f[index] = r[j];
            }
        }

        return f;
    }
    std::vector<unsigned> get_labels () const
    {
        std::vector<unsigned> l (dataset.size ());

        for (size_t i = 0; i < l.size (); ++i)
            l[i] = dataset.get_label (i);

        return l;
    }

    private:
    const T &dataset;
};

} // namespace coastnet
