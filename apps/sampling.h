#pragma once

#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/utils.h"

namespace ATL24_coastnet
{

namespace sampling_params
{
    const size_t patch_rows = 63;
    const size_t patch_cols = 7;
    const int64_t input_size = patch_rows * patch_cols;
    const double aspect_ratio = 8.0;
}

void print_sampling_params (std::ostream &os)
{
    os << "patch_rows: " << sampling_params::patch_rows << std::endl;
    os << "patch_cols: " << sampling_params::patch_cols << std::endl;
    os << "input_size: " << sampling_params::input_size << std::endl;
    os << "aspect_ratio: " << sampling_params::aspect_ratio << std::endl;
}

template<typename T>
class features
{
    public:
    explicit features (const T &dataset)
        : dataset (dataset)
    {
    }
    size_t size () const
    {
        return dataset.size ();
    }
    size_t features_per_sample () const
    {
        assert (dataset.size () != 0);
        // total features =
        //        photon elevation
        //      + raster size
        return    1
                + dataset.get_raster (0).size ();
    }
    std::vector<float> get_features () const
    {
        using namespace std;

        // Get all the features for this dataset
        vector<float> f;

        // Allocate space for features
        const size_t rows = dataset.size ();
        const size_t cols = features_per_sample ();
        f.reserve (rows * cols);

        // Stuff values into features vector
        for (size_t i = 0; i < rows; ++i)
        {
            // First feature is the photon's elevation
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

} // namespace ATL24_coastnet
