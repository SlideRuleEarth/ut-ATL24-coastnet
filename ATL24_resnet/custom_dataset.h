#pragma once

#include "ATL24_resnet/pgm.h"
#include "ATL24_resnet/utils.h"
#include "ATL24_utils/dataframe.h"

namespace ATL24_resnet
{

struct sample_index
{
    size_t dataset_index;
    size_t point_index;
};

class classified_point_dataset : public torch::data::datasets::Dataset<classified_point_dataset>
{
    using Example = torch::data::Example<>;
    std::vector<std::vector<ATL24_resnet::classified_point2d>> datasets;
    std::vector<sample_index> sample_indexes;
    size_t patch_size;
    double aspect_ratio;
    regularization_params rp;
    std::default_random_engine &rng;
    // The classifier wants the labels to be 0-based and sequential,
    // so remap the ASPRS labels during data loading
    std::unordered_map<long,long> label_map = {
        {0, 0},
        {7, 0},
        {2, 1},
        {4, 2},
        {5, 3},
        {41, 4},
        {45, 5},
        {40, 6},
    };

    public:
    classified_point_dataset (const std::vector<std::string> &fns,
        const size_t patch_size,
        const double aspect_ratio,
        const regularization_params &rp,
        const size_t samples_per_class,
        const bool verbose,
        std::default_random_engine &rng)
        : patch_size (patch_size)
        , aspect_ratio (aspect_ratio)
        , rp (rp)
        , rng (rng)
    {
        using namespace std;

        // One dataset per input file
        datasets.resize (fns.size ());

        // Open each file
        for (size_t i = 0; i < fns.size (); ++i)
        {
            const auto fn = fns[i];

            if (verbose)
                clog << "Reading " << fn << endl;

            // Read the points
            const auto df = ATL24_utils::dataframe::read (fn);

            // Convert them to the format that we want
            assert (df.is_valid ());
            assert (!df.headers.empty ());
            assert (!df.columns.empty ());

            // Get number of photons in this file
            const size_t nrows = df.columns[0].size ();

            // Get the columns we are interested in
            auto pi_it = find (df.headers.begin(), df.headers.end(), "ph_index");
            auto x_it = find (df.headers.begin(), df.headers.end(), "along_track_dist");
            auto z_it = find (df.headers.begin(), df.headers.end(), "egm08_orthometric_height");
            auto cls_it = find (df.headers.begin(), df.headers.end(), "manual_label");

            assert (pi_it != df.headers.end ());
            assert (x_it != df.headers.end ());
            assert (z_it != df.headers.end ());
            assert (cls_it != df.headers.end ());

            if (pi_it == df.headers.end ())
                throw runtime_error ("Can't find 'ph_index' in dataframe");
            if (x_it == df.headers.end ())
                throw runtime_error ("Can't find 'along_track_dist' in dataframe");
            if (z_it == df.headers.end ())
                throw runtime_error ("Can't find 'egm08_orthometric_height' in dataframe");
            if (cls_it == df.headers.end ())
                throw runtime_error ("Can't find 'manual_label' in dataframe");

            size_t ph_index = pi_it - df.headers.begin();
            size_t x_index = x_it - df.headers.begin();
            size_t z_index = z_it - df.headers.begin();
            size_t cls_index = cls_it - df.headers.begin();

            // Check logic
            assert (ph_index < df.headers.size ());
            assert (x_index < df.headers.size ());
            assert (z_index < df.headers.size ());
            assert (cls_index < df.headers.size ());
            assert (ph_index < df.columns.size ());
            assert (x_index < df.columns.size ());
            assert (z_index < df.columns.size ());
            assert (cls_index < df.columns.size ());

            // Stuff values into the vector
            datasets[i].resize (nrows);

            for (size_t j = 0; j < nrows; ++j)
            {
                // Check logic
                assert (j < df.columns[ph_index].size ());
                assert (j < df.columns[x_index].size ());
                assert (j < df.columns[z_index].size ());
                assert (j < df.columns[cls_index].size ());

                // Make assignments
                datasets[i][j].h5_index = df.columns[ph_index][j];
                datasets[i][j].x = df.columns[x_index][j];
                datasets[i][j].z = df.columns[z_index][j];
                datasets[i][j].cls = df.columns[cls_index][j];
            }

            if (verbose)
            {
                clog << datasets[i].size () << " points read" << endl;
                clog << "Sorting points" << endl;
            }

            // Sort them by X
            sort (datasets[i].begin (), datasets[i].end (),
                [](const auto &a, const auto &b)
                { return a.x < b.x; });
        }

        // Get sample indexes of points in each class
        unordered_map<size_t,vector<sample_index>> cls_indexes;

        for (size_t i = 0; i < datasets.size (); ++i)
            for (size_t j = 0; j < datasets[i].size (); ++j)
                cls_indexes[datasets[i][j].cls].push_back ({i, j});

        // Show results
        if (verbose)
        {
            clog << "Class counts:" << endl;
            for (auto i : cls_indexes)
                clog << "\t" << i.first << "\t" << i.second.size () << endl;
        }

        // For each class
        for (auto &i : cls_indexes)
        {
            // Randomize their orders
            shuffle (i.second.begin (), i.second.end (), rng);

            // Only keep the ones we need
            if (i.second.size () > samples_per_class)
                i.second.resize (samples_per_class);

            // Save them
            sample_indexes.insert (sample_indexes.end (), i.second.begin (), i.second.end ());
        }

        // Randomize the order
        shuffle (sample_indexes.begin (), sample_indexes.end (), rng);

        // Show results
        if (verbose)
            clog << "Total samples: " << sample_indexes.size () << endl;
    }

    Example get (size_t index)
    {
        using namespace std;
        using namespace viper::raster;

        assert (index < sample_indexes.size ());

        // Create image raster
        const auto dataset_index = sample_indexes[index].dataset_index;
        const auto point_index = sample_indexes[index].point_index;
        assert (dataset_index < datasets.size ());
        assert (point_index < datasets[dataset_index].size ());
        auto p = create_raster (
            datasets[dataset_index],
            point_index,
            patch_size,
            patch_size,
            aspect_ratio,
            rp,
            rng);

        // Create image Tensor from raster
        auto grayscale = torch::from_blob (&p[0],
                { static_cast<int> (patch_size), static_cast<int> (patch_size) },
                torch::kUInt8).to(torch::kFloat);

        // Create label Tensor
        const long cls = label_map.at (datasets[dataset_index][point_index].cls);
        auto label = torch::full ({1}, cls);

        return {grayscale.clone(), label.clone()};
    }

    torch::optional<size_t> size() const
    {
        return sample_indexes.size ();
    }
};

} // namespace ATL24_resnet
