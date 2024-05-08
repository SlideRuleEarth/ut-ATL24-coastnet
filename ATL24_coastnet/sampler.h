#pragma once

#include "raster.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ATL24_coastnet
{

namespace sampler
{

struct sample_parameters
{
    size_t random_seed = 123;
    size_t window_size = 10'000; // meters along track
    double x_resolution = 1.0 / 2.0; // meters per pixel
    double z_resolution = 1.0 / 16.0; // meters per pixel
    size_t patch_size = 512; // pixels on one size of a square
    size_t samples_per_segment = 64;
};

std::ostream &operator<< (std::ostream &os, const sample_parameters &sp)
{
    os << "\t" << "random_seed " << sp.random_seed << std::endl;
    os << "\t" << "window_size " << sp.window_size << std::endl;
    os << "\t" << "x_resolution " << sp.x_resolution << std::endl;
    os << "\t" << "z_resolution " << sp.z_resolution << std::endl;
    os << "\t" << "patch_size " << sp.patch_size << std::endl;
    os << "\t" << "samples_per_segment " << sp.samples_per_segment << std::endl;
    return os;
}

using image=ATL24_coastnet::raster::raster<unsigned char>;

template<typename SP,typename T>
std::vector<image> get_samples (const SP &sp,
    const T &p,
    const int raster_type,
    const double min_nonzero_percent,
    const bool normalize)
{
    using namespace std;

    // Create the random generator
    default_random_engine rng (sp.random_seed);

    // Get sample windows
    vector<pair<size_t,size_t>> windows;

    // Starting at the photon at 'i'
    for (size_t i = 0; i < p.size (); )
    {
        // Get 'window_size' meters worth of photons
        size_t j = i;
        while (j < p.size () && fabs (p[j].x - p[i].x) < sp.window_size)
            ++j;

        // Save this range
        windows.push_back (make_pair (i, j));

        // Continue to next range
        i = j;
    }

    // Return value
    vector<image> s;

    // Get samples from each range
    for (size_t i = 0; i < windows.size (); ++i)
    {
        // Create a raster from this range
        const auto r = create_raster (p,
            windows[i].first,
            windows[i].second,
            sp.x_resolution,
            sp.z_resolution,
            raster_type,
            // Normalize the sample, not the whole raster
            /* normalize = */ false);

        // Get samples from the raster
        for (size_t j = 0; j < sp.samples_per_segment; ++j)
        {
            // The raster is not guaranteed to be as tall or wide as a
            // sample patch. For example, some granules have big empty
            // areas with no photons, or we might be at the end of a
            // granule.
            const size_t row_end = r.rows () < sp.patch_size
                ? 0 : r.rows () - sp.patch_size;
            const size_t col_end = r.cols () < sp.patch_size
                ? 0 : r.cols () - sp.patch_size;

            // Create uniform distributions to select indexes
            //
            // Ranges are inclusize
            uniform_int_distribution<size_t> row_dist (0, row_end);
            uniform_int_distribution<size_t> col_dist (0, col_end);

            // Get offsets into the raster
            const auto random_i = row_dist (rng);
            const auto random_j = col_dist (rng);

            // Create an empty sample
            image sample (sp.patch_size, sp.patch_size);

            // Count non-zero samples
            size_t nonzero = 0;

            // Fill the sample
            for (size_t sample_i = 0; sample_i < sample.rows (); ++sample_i)
            {
                const auto raster_i = random_i + sample_i;
                // The patch may overlap the boundary of the raster
                if (raster_i >= r.rows ())
                    continue;
                for (size_t sample_j = 0; sample_j < sample.cols (); ++sample_j)
                {
                    const auto raster_j = random_j + sample_j;
                    // The patch may overlap the boundary of the raster
                    if (raster_j >= r.cols ())
                        continue;
                    sample (sample_i, sample_j) = r (raster_i, raster_j);

                    // Count
                    nonzero += (sample (sample_i, sample_j) != 0);
                }
            }

            const double nzp = 100.0 * nonzero / sample.size ();

            // Check non-zeroes
            if (nzp < min_nonzero_percent)
                continue;

            // Normalize if needed
            if (normalize)
            {
                const auto max = *max_element (sample.begin (), sample.end ());
                for_each (sample.begin (), sample.end (),
                    [&] (auto &x) { x = x * 255 / max; });
            }

            // Save the sample
            s.push_back (sample);
        }
    }

    return s;
}

} // namespace sampler

} // namespace ATL24_coastnet
