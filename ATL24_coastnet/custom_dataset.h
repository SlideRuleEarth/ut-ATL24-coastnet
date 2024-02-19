#pragma once

#include "ATL24_coastnet/pgm.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_utils/dataframe.h"
#include <map>

namespace ATL24_coastnet
{

struct sample_index
{
    size_t dataset_index;
    size_t point_index;
};

class classified_point_dataset : public torch::data::datasets::Dataset<classified_point_dataset>
{
    using Example = torch::data::Example<>;
    std::vector<std::vector<ATL24_coastnet::classified_point2d>> datasets;
    std::vector<sample_index> sample_indexes;
    std::vector<viper::raster::raster<unsigned char>> rasters;
    size_t patch_rows;
    size_t patch_cols;
    double aspect_ratio;
    augmentation_params ap;
    const bool ap_enabled;
    std::default_random_engine &rng;

    public:
    classified_point_dataset (const std::vector<std::string> &fns,
        const size_t patch_rows,
        const size_t patch_cols,
        const double aspect_ratio,
        const augmentation_params &ap,
        const bool ap_enabled,
        const size_t samples_per_class,
        const bool verbose,
        std::default_random_engine &rng)
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
            const auto df = ATL24_utils::dataframe::read (fn);

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

        using namespace viper::raster;

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

    Example get (size_t index)
    {
        using namespace std;

        // Create image Tensor from raster
        assert (index < rasters.size ());
        auto &p = rasters[index];
        auto grayscale = torch::from_blob (&p[0],
                { static_cast<int> (patch_rows), static_cast<int> (patch_cols) },
                torch::kUInt8).to(torch::kFloat);

        // Get sample indexes
        const auto dataset_index = sample_indexes[index].dataset_index;
        const auto point_index = sample_indexes[index].point_index;

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

class coastnet_surface_dataset : public torch::data::datasets::Dataset<coastnet_surface_dataset>
{
    using Example = torch::data::Example<>;
    std::vector<std::vector<ATL24_coastnet::classified_point2d>> datasets;
    std::vector<sample_index> sample_indexes;
    std::vector<viper::raster::raster<unsigned char>> rasters;
    size_t patch_rows;
    size_t patch_cols;
    double aspect_ratio;
    augmentation_params ap;
    const bool ap_enabled;
    std::default_random_engine &rng;

    public:
    coastnet_surface_dataset (const std::vector<std::string> &fns,
        const size_t patch_rows,
        const size_t patch_cols,
        const double aspect_ratio,
        const augmentation_params &ap,
        const bool ap_enabled,
        const size_t samples_per_class,
        const bool verbose,
        std::default_random_engine &rng)
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
#pragma omp parallel for
        for (size_t i = 0; i < fns.size (); ++i)
        {
            const auto fn = fns[i];

            if (verbose)
            {
#pragma omp critical
                clog << "Reading " << fn << endl;
            }

            // Read the points
            const auto df = ATL24_utils::dataframe::read (fn);

            // Convert it to the correct format
            datasets[i] = convert_dataframe (df);

            // Get a reference to this dataset
            auto &d = datasets[i];

            if (verbose)
            {
#pragma omp critical
                clog << d.size () << " points read" << endl;
            }

            // Sort them by X
            sort (d.begin (), d.end (),
                [](const auto &a, const auto &b)
                { return a.x < b.x; });

            // Force everything to be either surface or other
            for (auto &p : d)
                p.cls = (p.cls != surface_cls) ? 0 : surface_cls;

        }

        // Get sample indexes of points in each class
        unordered_map<size_t,vector<sample_index>> cls_indexes;

        for (size_t i = 0; i < datasets.size (); ++i)
        {
            for (size_t j = 0; j < datasets[i].size (); ++j)
            {
                // Only use samples at the correct elevation
                const auto p = datasets[i][j];
                if (p.z < surface_min_elevation || p.z > surface_max_elevation)
                    continue;
                cls_indexes[p.cls].push_back ({i, j});
            }
        }

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

        using namespace viper::raster;

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

    Example get (size_t index)
    {
        using namespace std;

        // Create image Tensor from raster
        assert (index < rasters.size ());
        auto &p = rasters[index];
        auto grayscale = torch::from_blob (&p[0],
                { static_cast<int> (patch_rows), static_cast<int> (patch_cols) },
                torch::kUInt8).to(torch::kFloat);

        // Get sample indexes
        const auto dataset_index = sample_indexes[index].dataset_index;
        const auto point_index = sample_indexes[index].point_index;

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

} // namespace ATL24_coastnet
