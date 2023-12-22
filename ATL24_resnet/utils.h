#pragma once

#include "viper/raster.h"
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ATL24_resnet
{

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
};

struct classified_point3d
{
    size_t h5_index;
    double ts, lat, lon, z;
    size_t cls;
};

std::vector<classified_point2d> read_classified_point2d (std::istream &is)
{
    using namespace std;

    string line;

    // Get the column labels
    getline (is, line);

    vector<classified_point2d> p;
    while (getline (is, line))
    {
        stringstream ss (line);
        size_t h5_index;
        ss >> h5_index;
        ss.get (); // ignore ','
        double x;
        ss >> x;
        ss.get (); // ignore ','
        double z;
        ss >> z;
        ss.get (); // ignore ','
        size_t cls;
        ss >> cls;
        p.push_back (classified_point2d {h5_index, x, z, cls});
    }

    return p;
}

template<typename T>
void write_point2d (std::ostream &os, const T &p)
{
    using namespace std;

    // Print along-track meters
    os << "ph_index,along_track_dist,geoid_corrected_h,manual_label" << endl;
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Write the index
        os << setprecision (8) << fixed;
        os << p[i].h5_index;
        // Double has 15 decimal digits of precision
        os << setprecision (15) << fixed;
        os << "," << p[i].x;
        // Elevation was corrected from EGM, which is a 32 bit float,
        // so it only has about 8 decimal digits of precision.
        os << setprecision (8) << fixed;;
        os << "," << p[i].z;
        os << ",0"; // 0=unlabeled
        os << endl;
    }
}

template<typename T>
void write_classified_point2d (std::ostream &os, const T &p)
{
    using namespace std;

    // Print along-track meters
    os << "ph_index,along_track_dist,geoid_corrected_h,manual_label" << endl;
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Write the index
        os << setprecision (8) << fixed;
        os << p[i].h5_index;
        // Double has 15 decimal digits of precision
        os << setprecision (15) << fixed;;
        os << "," << p[i].x;
        // Elevation was corrected from EGM, which is a 32 bit float,
        // so it only has about 8 decimal digits of precision.
        os << setprecision (8) << fixed;;
        os << "," << p[i].z;
        // Write the class
        os << "," << p[i].cls;
        os << endl;
    }
}

struct point2d_extents
{
    point2d minp;
    point2d maxp;
};

template<typename T>
point2d_extents get_extents (const T &p,
    const size_t start_index,
    const size_t end_index)
{
    // Check invariants
    assert (start_index < end_index);

    point2d_extents e;

    e.minp.x = min_element (p.begin () + start_index, p.begin () + end_index,
        [](const auto &a, const auto &b)
        { return a.x < b.x; })->x;
    e.maxp.x = max_element (p.begin () + start_index, p.begin () + end_index,
        [](const auto &a, const auto &b)
        { return a.x < b.x; })->x;
    e.minp.z = min_element (p.begin () + start_index, p.begin () + end_index,
        [](const auto &a, const auto &b)
        { return a.z < b.z; })->z;
    e.maxp.z = max_element (p.begin () + start_index, p.begin () + end_index,
        [](const auto &a, const auto &b)
        { return a.z < b.z; })->z;

    return e;
}

template<typename T>
point2d_extents get_extents (const T &p)
{
    return get_extents (p, 0, p.size ());
}

std::pair<size_t,size_t> get_grid_location (const auto &p,
    const auto &minp,
    const double x_resolution,
    const double z_resolution)
{
    const size_t row = (p.z - minp.z) / z_resolution;
    const size_t col = (p.x - minp.x) / x_resolution;
    return std::make_pair (row, col);
}

struct regularization_params
{
    bool enabled = true;
    double jitter_std = 0.1; // standard deviation in meters
    double scale_x_min = 0.9; // meters
    double scale_x_max = 1.1; // meters
    double scale_z_min = 0.9; // meters
    double scale_z_max = 1.1; // meters
    bool mirror = true;
};

template<typename T>
viper::raster::raster<unsigned char> create_raster (const T &p,
    const size_t index,
    const size_t rows,
    const size_t cols,
    const double aspect_ratio,
    const regularization_params &rp,
    std::default_random_engine &rng)
{
    using namespace std;

    // Check invariants
    assert (index < p.size ());

    // Create an empty raster
    viper::raster::raster<unsigned char> r (rows, cols);

    // The point at 'index' will be centered in the patch
    //
    // Get size of patch in meters
    const double width = cols * aspect_ratio;

    // Find the left and right boundaries along the X axis of the patch
    size_t index_left = index;
    size_t index_right = index;

    // Left boundary
    while (index_left != 0)
    {
        // The values should be sorted by 'X'
        assert (p[index].x >= p[index_left].x);

        // Check the left boundary of the patch
        if ((p[index].x - p[index_left].x) > width / 2.0)
            break;

        --index_left;
    }

    // Right boundary
    while (index_right < p.size ())
    {
        // The values should be sorted by 'X'
        assert (p[index_right].x >= p[index].x);

        // Check the right boundary of the patch
        if ((p[index_right].x - p[index].x) > width / 2.0)
            break;

        ++index_right;
    }

    // Create regularization distributions
    normal_distribution<double> jitter_dist (0.0, rp.jitter_std);
    uniform_real_distribution<double> scale_x_dist (rp.scale_x_min, rp.scale_x_max);
    uniform_real_distribution<double> scale_y_dist (rp.scale_z_min, rp.scale_z_max);
    bernoulli_distribution mirror_dist (rp.mirror ? 0.5 : 0.0);

    // Mirror all points or none
    const auto mirror = mirror_dist (rng);

    // Get location of center point of the patch
    const double x0 = p[index].x;
    const double z0 = p[index].z;

    // Look at all of the points between left and right indexes to
    // determine where they go in the patch
    for (size_t i = index_left; i < index_right; ++i)
    {
        // Get the point's location in the patch
        const double x = p[i].x;
        const double z = p[i].z;

        // Where is it relative to the patch center in meters?
        double dx = x - x0;
        double dz = z - z0;

        // Apply regularization
        if (rp.enabled)
        {
            dx += jitter_dist (rng);
            dz += jitter_dist (rng);
            dx *= scale_x_dist (rng);
            dz *= scale_y_dist (rng);
            dx = mirror ? -dx : dx;
        }

        // Convert from meters to a patch index
        const size_t patch_i = dz + (static_cast<double> (rows) / 2.0);
        const size_t patch_j = (dx / aspect_ratio) + (static_cast<double> (cols) / 2.0);

        // Check bounds
        if (patch_j >= r.cols ())
            continue;
        if (patch_i >= r.rows ())
            continue;

        // 0 == no data
        //
        // Color according to its distance from sea level
        if (z > 0.5 || z < -0.5)
            r (patch_i, patch_j) = 1;
        else
            r (patch_i, patch_j) = 2;
    }

    return r;
}

template<typename T>
std::vector<ATL24_resnet::classified_point2d> convert_dataframe (const T &df)
{
    using namespace std;

    // Check invariants
    assert (df.is_valid ());
    assert (!df.headers.empty ());
    assert (!df.columns.empty ());

    // Get number of photons in this file
    const size_t nrows = df.columns[0].size ();

    // Get the columns we are interested in
    auto pi_it = find (df.headers.begin(), df.headers.end(), "ph_index");
    auto x_it = find (df.headers.begin(), df.headers.end(), "along_track_dist");
    auto z_it = find (df.headers.begin(), df.headers.end(), "geoid_corrected_h");
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
        throw runtime_error ("Can't find 'geoid_corrected_h' in dataframe");
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
    std::vector<ATL24_resnet::classified_point2d> dataset (nrows);

    for (size_t j = 0; j < nrows; ++j)
    {
        // Check logic
        assert (j < df.columns[ph_index].size ());
        assert (j < df.columns[x_index].size ());
        assert (j < df.columns[z_index].size ());
        assert (j < df.columns[cls_index].size ());

        // Make assignments
        dataset[j].h5_index = df.columns[ph_index][j];
        dataset[j].x = df.columns[x_index][j];
        dataset[j].z = df.columns[z_index][j];
        dataset[j].cls = df.columns[cls_index][j];
    }

    return dataset;
}

} // namespace ATL24_resnet
