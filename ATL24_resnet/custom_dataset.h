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

            // Convert it to the correct format
            datasets[i] = convert_dataframe (df);

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
