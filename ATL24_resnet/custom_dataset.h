#pragma once

#include "ATL24_resnet/pgm.h"
#include "ATL24_resnet/utils.h"
#include "ATL24_utils/dataframe.h"
#include <map>

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
    size_t patch_rows;
    size_t patch_cols;
    double aspect_ratio;
    augmentation_params ap;
    std::default_random_engine &rng;

    public:
    classified_point_dataset (const std::vector<std::string> &fns,
        const size_t patch_rows,
        const size_t patch_cols,
        const double aspect_ratio,
        const augmentation_params &ap,
        const size_t samples_per_class,
        const bool verbose,
        std::default_random_engine &rng)
        : patch_rows (patch_rows)
        , patch_cols (patch_cols)
        , aspect_ratio (aspect_ratio)
        , ap (ap)
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
            patch_rows,
            patch_cols,
            aspect_ratio,
            ap,
            rng);

        // Create image Tensor from raster
        auto grayscale = torch::from_blob (&p[0],
                { static_cast<int> (patch_rows), static_cast<int> (patch_cols) },
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

constexpr unsigned other_cls = 0;
constexpr unsigned bathy_cls = 40;
constexpr unsigned surface_cls = 41;

class coastnet_dataset : public torch::data::datasets::Dataset<coastnet_dataset>
{
    using Example = torch::data::Example<>;
    std::vector<std::vector<ATL24_resnet::classified_point2d>> datasets;
    std::vector<sample_index> sample_indexes;
    size_t patch_rows;
    size_t patch_cols;
    double aspect_ratio;
    augmentation_params ap;
    std::default_random_engine &rng;

    public:
    coastnet_dataset (const std::vector<std::string> &fns,
        const size_t patch_rows,
        const size_t patch_cols,
        const double aspect_ratio,
        const augmentation_params &ap,
        const size_t max_samples_per_class,
        const bool verbose,
        std::default_random_engine &rng)
        : patch_rows (patch_rows)
        , patch_cols (patch_cols)
        , aspect_ratio (aspect_ratio)
        , ap (ap)
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

            // Get a reference to this dataset
            auto &d = datasets[i];

            if (verbose)
                clog << d.size () << " points read" << endl;

            // Sort them by X
            sort (d.begin (), d.end (),
                [](const auto &a, const auto &b)
                { return a.x < b.x; });

            // Count total in each class
            std::map<size_t,size_t> cls_counts;

            // Force classifications to be one of three
            for (auto &p : d)
            {
                switch (p.cls)
                {
                    default:
                        // Force it to 'other'
                        p.cls = 0;
                    break;
                    case bathy_cls:
                    case surface_cls:
                        // Leave it alone
                    break;
                }

                // Count it
                ++cls_counts[p.cls];
            }

            // Keep track of how many got added from each class
            unordered_map<unsigned,size_t> left_to_add;
            left_to_add[other_cls] = max_samples_per_class * 2;
            left_to_add[bathy_cls] = max_samples_per_class * 1;
            left_to_add[surface_cls] = max_samples_per_class * 3;

            // Indexes into the dataset points
            vector<size_t> indexes (d.size ());

            // 0, 1, 2, ...
            iota (indexes.begin (), indexes.end (), 0);

            // Go through the dataset in a random order
            shuffle (indexes.begin (), indexes.end (), rng);

            for (size_t j = 0; j < indexes.size (); ++j)
            {
                // Get the index
                const auto k = indexes[j];

                // Get the point
                const auto p = d[k];

                // Still more to add from this class?
                if (left_to_add[p.cls] == 0)
                    continue; // No, go to next point

                // Keep track of how many were added
                --left_to_add[p.cls];

                // Append it
                sample_indexes.push_back (sample_index {i, k});
            }
        }

        // Get sample indexes of points in each class
        std::map<size_t,size_t> cls_counts;

        for (size_t i = 0; i < sample_indexes.size (); ++i)
        {
            const size_t dindex = sample_indexes[i].dataset_index;
            assert (dindex < datasets.size ());
            const size_t pindex = sample_indexes[i].point_index;
            assert (pindex < datasets[dindex].size ());
            const auto cls = datasets[dindex][pindex].cls;
            ++cls_counts[cls];
        }

        // Show results
        if (verbose)
        {
            clog << "Class counts:" << endl;
            for (auto i : cls_counts)
                clog << "\t" << i.first << "\t" << i.second << endl;

            clog << "Total samples: " << sample_indexes.size () << endl;
        }
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
            patch_rows,
            patch_cols,
            aspect_ratio,
            ap,
            rng);

        // Create image Tensor from raster
        auto grayscale = torch::from_blob (&p[0],
                { static_cast<int> (patch_rows), static_cast<int> (patch_cols) },
                torch::kUInt8).to(torch::kFloat);

        // Get the class
        const auto cls = datasets[dataset_index][point_index].cls;

        // Remap label
        long c = 0;
        switch (cls)
        {
            default: assert (cls == 0); break;
            case bathy_cls: c = 1; break;
            case surface_cls: c = 2; break;
        }

        // Create label Tensor
        auto label = torch::full ({1}, c);

        // Return copies
        return {grayscale.clone(), label.clone()};
    }

    torch::optional<size_t> size() const
    {
        return sample_indexes.size ();
    }
};

} // namespace ATL24_resnet
