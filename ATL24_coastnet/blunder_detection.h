#pragma once

#include "precompiled.h"

namespace ATL24_coastnet
{

// ASPRS Definitions
constexpr unsigned bathy_class = 40;
constexpr unsigned sea_surface_class = 41;

namespace detail
{

template<typename T>
T surface_elevation_check (T p,
    const double surface_min_elevation,
    const double surface_max_elevation)
{
    assert (!p.empty ());

    // Surface photons must be near sea level
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-surface
        if (p[i].prediction != sea_surface_class)
            continue;

        // Too high?
        if (p[i].z > surface_max_elevation)
            p[i].prediction = 0;

        // Too low?
        if (p[i].z < surface_min_elevation)
            p[i].prediction = 0;
    }

    return p;
}

template<typename T>
T bathy_elevation_check (T p,
    const double bathy_min_elevation)
{
    assert (!p.empty ());

    // Bathy photons can't be too deep
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].prediction != bathy_class)
            continue;

        // Too deep?
        if (p[i].z < bathy_min_elevation)
            p[i].prediction = 0;
    }

    return p;
}

template<typename T>
std::vector<double> get_quantized_variance (const T &p, const unsigned cls, const double bin_size)
{
    using namespace std;

    assert (!p.empty ());

    // Get extent
    const unsigned min_x = p[0].x;
    const unsigned max_x = p.back ().x + bin_size;

    // Values should be sorted
    assert (min_x < max_x);
    const size_t total = (max_x - min_x) / bin_size;

    // Get 1m window averages
    vector<double> sums (total);
    vector<double> sums2 (total);
    vector<double> totals (total);
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Skip non-'cls' photons
        if (p[i].prediction != cls)
            continue;

        // Get along-track index
        const double distance = (p[i].x - min_x) / bin_size;
        assert (distance >= 0.0);
        const unsigned j = std::floor (distance);

        // Check logic
        assert (j < sums.size ());
        assert (j < sums2.size ());
        assert (j < totals.size ());

        // Count the value
        sums[j] += p[i].z;
        sums2[j] += (p[i].z * p[i].z);
        ++totals[j];
    }

    // Compute the variance of 'cls' at each photon
    vector<double> var (p.size (), NAN);
    for (size_t i = 0; i < var.size (); ++i)
    {
        // Get along-track index
        const double distance = (p[i].x - min_x) / bin_size;
        assert (distance >= 0.0);
        const unsigned j = std::floor (distance);

        // Check logic
        assert (j < sums.size ());
        assert (j < sums2.size ());
        assert (j < totals.size ());

        if (totals[j] == 0)
            continue; // Leave it NAN

        // E(X) = E(X^2) - E(X)^2
        const double ex = sums[j] / totals[j];
        const double ex2 = sums2[j] / totals[j];

        // Check for rounding error
        assert (i < var.size ());
        if (ex2 < ex * ex)
            var[i] = 0.0;
        else
            var[i] = ex2 - ex * ex;

        // Logic check
        assert (var[i] >= 0.0);
        assert (!std::isnan (var[i]));
    }

    return var;
}

template<typename T>
T bathy_depth_check (T p, const double surface_bin_size, const double surface_depth_factor)
{
    assert (!p.empty ());

    // Count bathy photons
    const size_t total_bathy = count_predictions (p, bathy_class);

    // If there is no bathy, there is nothing to do
    if (total_bathy == 0)
        return p;

    // Count surface photons
    const size_t total_surface = count_predictions (p, sea_surface_class);

    // If there is no surface, there can't be any bathy
    if (total_surface == 0)
    {
        for (size_t i = 0; i < p.size (); ++i)
            p[i].prediction = 0;

        return p;
    }

    // Compute sea surface variance
    const auto var = get_quantized_variance (p, sea_surface_class, surface_bin_size);

    // Variance is computed at each photon
    assert (var.size () == p.size ());

    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].prediction != bathy_class)
            continue;

        // If there is no sea surface above it, this can't be bathy
        if (std::isnan (var[i]))
        {
            p[i].prediction = 0;
            continue;
        }

        // Is the bathy deep enough below the surface?
        //
        // It must be 'surface_depth_factor' stddev's below the surface, but
        // but clamp the buffer depth to 1.0m.
        const double surface_stddev = std::sqrt (var[i]);
        const double min_depth =
            surface_depth_factor * surface_stddev > 1.0
            ? 1.0
            : surface_depth_factor * surface_stddev;
        const double bathy_min_depth = p[i].surface_elevation - min_depth;

        if (p[i].z > bathy_min_depth)
        {
            p[i].prediction = 0;
            continue;
        }
    }

    return p;
}

template<typename T>
T surface_range_check (T p, const double range)
{
    assert (!p.empty ());

    // Count surface photons
    const size_t total_surface = count_predictions (p, sea_surface_class);

    // If there is no surface, there is nothing to do
    if (total_surface == 0)
        return p;

    // Surface photons must be near the surface estimate
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-surface
        if (p[i].prediction != sea_surface_class)
            continue;

        // Is it close enough?
        const double d = std::fabs (p[i].z - p[i].surface_elevation);

        // Must be within +-range
        if (d > range)
            p[i].prediction = 0;
    }

    return p;
}

template<typename T>
T bathy_range_check (T p, const double range)
{
    assert (!p.empty ());

    // Count bathy photons
    const size_t total_bathy = count_predictions (p, bathy_class);

    // If there is no bathy, there is nothing to do
    if (total_bathy == 0)
        return p;

    // Bathy photons must be near the bathy estimate
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].prediction != bathy_class)
            continue;

        // Is it close enough?
        const double d = std::fabs (p[i].z - p[i].bathy_elevation);

        // Must be within +-range
        if (d > range)
            p[i].prediction = 0;
    }

    return p;
}

} // namespace detail

template<typename T,typename U>
T blunder_detection (T p, const U &params)
{
    // Reclassify photons using heuristics
    using namespace std;

    if (p.empty ())
        return p;

    // Surface photons must be near sea level
    p = detail::surface_elevation_check (p,
        params.surface_min_elevation,
        params.surface_max_elevation);

    // Bathy photons can't be too deep
    p = detail::bathy_elevation_check (p,
        params.bathy_min_elevation);

    // Bathy photons can't be above the sea surface
    p = detail::bathy_depth_check (p, params.blunder_surface_bin_size, params.blunder_surface_depth_factor);

    // Sea surface photons must all be near the elevation estimate
    p = detail::surface_range_check (p, params.surface_range);

    // Bathy photons must all be near the elevation estimate
    p = detail::bathy_range_check (p, params.bathy_range);

    return p;
}

} // namespace ATL24_coastnet
