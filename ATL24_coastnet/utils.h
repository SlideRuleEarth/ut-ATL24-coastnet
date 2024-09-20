#pragma once

#include "coastnet.h"
#include "raster.h"

const std::string PI_NAME ("index_ph");
const std::string X_NAME ("x_atc");
const std::string Z_NAME ("ortho_h");

#ifndef PREDICTION_NAME
#define PREDICTION_NAME "prediction"
#endif

namespace ATL24_coastnet
{

const std::string LABEL_NAME = std::string ("manual_label");
const std::string SEA_SURFACE_NAME = std::string ("sea_surface_h");
const std::string BATHY_NAME = std::string ("bathy_h");

struct photon_hash
{
    std::size_t operator() (const std::pair<long,long> &v) const
    {
        size_t hash = (v.first << 0) ^ (v.second << 32);
        return hash;
    }
};

class prediction_cache
{
    public:
    template<typename T>
    bool check (const T &p, const size_t i) const
    {
        // Get photon coordinate
        const auto h = photon_coord (p, i);

        // Check the map
        if (m.find (h) != m.end ())
            return true;
        else
            return false;
    }
    template<typename T>
    long get_prediction (const T &p, const size_t i) const
    {
        // Get photon coordinate
        const auto h = photon_coord (p, i);

        // Check logic
        assert (m.find (h) != m.end ());

        // Get the value
        return m.at (h);
    }
    template<typename T>
    void update (const T &p, const size_t i, const long prediction)
    {
        // Get photon coordinate
        const auto h = photon_coord (p, i);

        // Set the value
        m[h] = prediction;
    }

    private:
    std::unordered_map<std::pair<long,long>, long, photon_hash> m;
    const double x_resolution = 0.5;
    const double z_resolution = 0.5;

    // Compute a hash from a photon's location
    template<typename T>
    std::pair<long,long> photon_coord (const T &p, const size_t i) const
    {
        // Check invariants
        assert (i < p.size ());

        // Quantize
        const long x = std::round (p[i].x / x_resolution);
        const long z = std::round (p[i].z / z_resolution);

        return std::make_pair (x, z);
    };
};

template<typename T>
void write_point2d (std::ostream &os, const T &p)
{
    using namespace std;

    // Save precision
    const auto pr = os.precision ();

    // Print along-track meters
    os << "index_ph,x_atc,ortho_h,manual_label" << endl;
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Write the index
        os << fixed;
        os << p[i].h5_index;
        os << setprecision (4) << fixed;
        os << "," << p[i].x;
        os << setprecision (4) << fixed;;
        os << "," << p[i].z;
        os << ",0"; // 0=unlabeled
        os << endl;
    }

    // Restore precision
    os.precision (pr);
}

template<typename T>
void write_classified_point2d (std::ostream &os, const T &p)
{
    using namespace std;

    // Save precision
    const auto pr = os.precision ();

    // Print along-track meters
    os << "index_ph,x_atc,ortho_h,manual_label,prediction,sea_surface_h,bathy_h" << endl;
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Write the index
        os << fixed;
        os << p[i].h5_index;
        os << setprecision (4) << fixed;;
        os << "," << p[i].x;
        os << setprecision (4) << fixed;;
        os << "," << p[i].z;
        // Write the class
        os << setprecision (0) << fixed;;
        os << "," << p[i].cls;
        // Write the prediction
        os << setprecision (0) << fixed;
        os << "," << p[i].prediction;
        // Write the surface estimate
        os << setprecision (4) << fixed;
        os << "," << p[i].surface_elevation;
        // Write the bathy estimate
        os << setprecision (4) << fixed;
        os << "," << p[i].bathy_elevation;
        os << endl;
    }

    // Restore precision
    os.precision (pr);
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

struct augmentation_params
{
    double jitter_x_std = 0.0001; // standard deviation in meters
    double jitter_z_std = 0.1; // standard deviation in meters
    double scale_x_min = 0.9; // scale factor
    double scale_x_max = 1.1; // scale factor
    double scale_z_min = 0.9; // scale factor
    double scale_z_max = 1.1; // scale factor
    double mirror_probabilty = 0.5;
};

std::ostream &operator<< (std::ostream &os, const augmentation_params &ap)
{
    os << "jitter_x_std: " << ap.jitter_x_std << std::endl;
    os << "jitter_z_std: " << ap.jitter_z_std << std::endl;
    os << "scale_x_min: " << ap.scale_x_min << std::endl;
    os << "scale_x_max: " << ap.scale_x_max << std::endl;
    os << "scale_z_min: " << ap.scale_z_min << std::endl;
    os << "scale_z_max: " << ap.scale_z_max << std::endl;
    os << "mirror_probabilty: " << ap.mirror_probabilty << std::endl;
    return os;
}

template<typename T>
ATL24_coastnet::raster::raster<unsigned char> create_raster (const T &p,
    const size_t index,
    const size_t rows,
    const size_t cols,
    const double aspect_ratio,
    const augmentation_params &ap = augmentation_params {},
    const bool ap_enabled = false,
    const size_t random_seed = 0)
{
    using namespace std;

    // Create random number generator
    std::default_random_engine rng (random_seed);

    // Check invariants
    assert (index < p.size ());

    // Create an empty raster
    ATL24_coastnet::raster::raster<unsigned char> r (rows, cols);

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

    // Create augmentation distributions
    normal_distribution<double> jitter_x_dist (0.0, ap.jitter_x_std);
    normal_distribution<double> jitter_z_dist (0.0, ap.jitter_z_std);
    uniform_real_distribution<double> scale_x_dist (ap.scale_x_min, ap.scale_x_max);
    uniform_real_distribution<double> scale_z_dist (ap.scale_z_min, ap.scale_z_max);
    bernoulli_distribution mirror_dist (ap.mirror_probabilty);

    // Mirror all points or none
    const auto mirror = mirror_dist (rng);

    // Scale all points the same
    const auto scale_x = scale_x_dist (rng);
    const auto scale_z = scale_z_dist (rng);

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

        // Apply augmentation
        if (ap_enabled)
        {
            dx += jitter_x_dist (rng);
            dz += jitter_z_dist (rng);
            dx *= scale_x;
            dz *= scale_z;
            dx = mirror ? -dx : dx;
        }

        // Convert from meters to a patch index
        const size_t patch_i = dz + rows / 2.0;
        const size_t patch_j = (dx / aspect_ratio) + cols / 2.0;

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
std::vector<ATL24_coastnet::classified_point2d> convert_dataframe (
    const T &df,
    bool &has_manual_label,
    bool &has_predictions,
    bool &has_surface_elevations,
    bool &has_bathy_elevations)
{
    using namespace std;

    // Check invariants
    assert (df.is_valid ());
    assert (df.rows () != 0);
    assert (df.cols () != 0);

    // Get number of photons in this file
    const size_t nrows = df.rows ();

    // Get the columns we are interested in
    const auto headers = df.get_headers ();
    auto pi_it = find (headers.begin(), headers.end(), PI_NAME);
    auto x_it = find (headers.begin(), headers.end(), X_NAME);
    auto z_it = find (headers.begin(), headers.end(), Z_NAME);
    auto cls_it = find (headers.begin(), headers.end(), LABEL_NAME);
    auto prediction_it = find (headers.begin(), headers.end(), PREDICTION_NAME);
    auto surface_elevation_it = find (headers.begin(), headers.end(), SEA_SURFACE_NAME);
    auto bathy_elevation_it = find (headers.begin(), headers.end(), BATHY_NAME);

    if (pi_it == headers.end ())
        throw runtime_error ("Can't find 'index_ph' in dataframe");
    if (x_it == headers.end ())
        throw runtime_error ("Can't find 'x_atc' in dataframe");
    if (z_it == headers.end ())
        throw runtime_error ("Can't find 'ortho_h' in dataframe");

    has_manual_label = cls_it != headers.end ();
    has_predictions = prediction_it != headers.end ();
    has_surface_elevations = surface_elevation_it != headers.end ();
    has_bathy_elevations = bathy_elevation_it != headers.end ();

    // Stuff values into the vector
    std::vector<ATL24_coastnet::classified_point2d> dataset (nrows);

    for (size_t i = 0; i < nrows; ++i)
    {
        // Make assignments
        dataset[i].h5_index = df.get_value (PI_NAME, i);
        dataset[i].x = df.get_value (X_NAME, i);
        dataset[i].z = df.get_value (Z_NAME, i);
        if (has_manual_label)
            dataset[i].cls = df.get_value (LABEL_NAME, i);
        if (has_predictions)
            dataset[i].prediction = df.get_value (PREDICTION_NAME, i);
        if (has_surface_elevations)
            dataset[i].surface_elevation = df.get_value (SEA_SURFACE_NAME, i);
        if (has_bathy_elevations)
            dataset[i].bathy_elevation = df.get_value (BATHY_NAME, i);
    }

    return dataset;
}

template<typename T>
std::vector<ATL24_coastnet::classified_point2d> convert_dataframe (const T &df)
{
    bool has_manual_label;
    bool has_predictions;
    bool has_surface_elevations;
    bool has_bathy_elevations;

    return convert_dataframe (df,
        has_manual_label,
        has_predictions,
        has_surface_elevations,
        has_bathy_elevations);
}

template<typename T>
std::vector<ATL24_coastnet::classified_point2d> convert_dataframe (const T &df,
    bool &has_manual_label,
    bool &has_predictions)
{
    bool has_surface_elevations;
    bool has_bathy_elevations;

    return convert_dataframe (df,
        has_manual_label,
        has_predictions,
        has_surface_elevations,
        has_bathy_elevations);
}

} // namespace ATL24_coastnet
