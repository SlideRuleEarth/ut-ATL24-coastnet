#pragma once

#include "coastnet.h"
#include "dataframe.h"
#include "raster.h"
#include "utils.h"

namespace ATL24_coastnet
{

struct sample_index
{
    size_t dataset_index;
    size_t point_index;
};

template<typename RNG>
class coastnet_dataset
{
    std::vector<std::vector<ATL24_coastnet::classified_point2d>> datasets;
    std::vector<sample_index> sample_indexes;
    std::vector<raster::raster<unsigned char>> rasters;
    size_t patch_rows;
    size_t patch_cols;
    double aspect_ratio;
    augmentation_params ap;
    const bool ap_enabled;
    RNG &rng;

    public:
    coastnet_dataset (const std::vector<std::string> &fns,
        const size_t patch_rows,
        const size_t patch_cols,
        const double aspect_ratio,
        const augmentation_params &ap,
        const bool ap_enabled,
        const size_t samples_per_class,
        const bool verbose,
        RNG &rng)
        : patch_rows (patch_rows)
        , patch_cols (patch_cols)
        , aspect_ratio (aspect_ratio)
        , ap (ap)
        , ap_enabled (ap_enabled)
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
            const auto df = ATL24_coastnet::dataframe::read (fn);

            // Convert it to the correct format
            datasets[i] = convert_dataframe (df);

            if (verbose)
                clog << datasets[i].size () << " points read" << endl;

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

        if (verbose)
            clog << "Creating rasters..." << endl;

        // Allocate vector
        rasters.resize (sample_indexes.size ());

        // Create seeds
        vector<size_t> random_seeds (rasters.size ());
        for (size_t i = 0; i < random_seeds.size (); ++i)
            random_seeds[i] = rng ();

        using namespace ATL24_coastnet::raster;

#pragma omp parallel for
        for (size_t i = 0; i < rasters.size (); ++i)
        {
            const auto dataset_index = sample_indexes[i].dataset_index;
            const auto point_index = sample_indexes[i].point_index;
            assert (dataset_index < datasets.size ());
            assert (point_index < datasets[dataset_index].size ());
            rasters[i] = create_raster (
                datasets[dataset_index],
                point_index,
                patch_rows,
                patch_cols,
                aspect_ratio,
                ap,
                ap_enabled,
                random_seeds[i]);
        }

        // Show results
        if (verbose)
            clog << "Total samples: " << sample_indexes.size () << endl;
    }

    ATL24_coastnet::raster::raster<unsigned char> get_raster (size_t index) const
    {
        using namespace std;

        assert (index < rasters.size ());
        return rasters[index];
    }

    unsigned get_label (size_t index) const
    {
        using namespace std;

        assert (index < sample_indexes.size ());

        // Get sample indexes
        const auto dataset_index = sample_indexes[index].dataset_index;
        const auto point_index = sample_indexes[index].point_index;

        assert (dataset_index < datasets.size ());
        assert (point_index < datasets[dataset_index].size ());

        return label_map.at (datasets[dataset_index][point_index].cls);
    }

    double get_elevation (size_t index) const
    {
        using namespace std;

        assert (index < sample_indexes.size ());

        // Get sample indexes
        const auto dataset_index = sample_indexes[index].dataset_index;
        const auto point_index = sample_indexes[index].point_index;

        assert (dataset_index < datasets.size ());
        assert (point_index < datasets[dataset_index].size ());

        return datasets[dataset_index][point_index].z;
    }

    size_t size() const
    {
        return sample_indexes.size ();
    }
};

} // namespace ATL24_coastnet
