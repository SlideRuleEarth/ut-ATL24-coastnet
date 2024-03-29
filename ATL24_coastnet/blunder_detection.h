#pragma once

#include "ATL24_coastnet/coastnet.h"
#include <vector>

namespace ATL24_coastnet
{

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
            p[i].prediction = unclassified_class;

        // Too low?
        if (p[i].z < surface_min_elevation)
            p[i].prediction = unclassified_class;
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
            p[i].prediction = unclassified_class;
    }

    return p;
}

template<typename T>
T relative_depth_check (T p, const double water_column_width)
{
    assert (!p.empty ());

    // Count surface photons
    const size_t total_surface = count_predictions (p, sea_surface_class);

    // If there is no surface, there is nothing to do
    if (total_surface == 0)
        return p;

    // Count bathy photons
    const size_t total_bathy = count_predictions (p, bathy_class);

    // If there is no bathy, there is nothing to do
    if (total_bathy == 0)
        return p;

    // We need to know the along-track distance to surface photons
    const auto nearby_surface_indexes = get_nearest_along_track_prediction (p, sea_surface_class);

    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].prediction != bathy_class)
            continue;

        // Get the closest surface index
        const size_t j = nearby_surface_indexes[i];

        // How far away is it?
        assert (j < p.size ());
        const double dx = fabs (p[i].x - p[j].x);

        // If it's too far away, we can't do the check
        if (dx > water_column_width)
            continue;

        // If the bathy below the surface?
        if (p[i].z < p[j].surface_elevation)
            continue; // Yes, keep going

        // No, reassign
        p[i].prediction = unclassified_class;
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
            p[i].prediction = unclassified_class;
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
            p[i].prediction = unclassified_class;
    }

    return p;
}

} // namespace detail

template<typename T,typename U>
T blunder_detection (T p, const U &args)
{
    // Reclassify photons using heuristics
    using namespace std;

    if (p.empty ())
        return p;

    // Count classes
    const size_t total_surface = count_predictions (p, sea_surface_class);
    const size_t total_bathy = count_predictions (p, bathy_class);

    if (args.verbose)
    {
        clog << "Total surface photons: " << total_surface << endl;
        clog << "Total bathy photons: " << total_bathy << endl;
    }

    // Surface photons must be near sea level
    p = detail::surface_elevation_check (p,
        args.surface_min_elevation,
        args.surface_max_elevation);

    // Bathy photons can't be too deep
    p = detail::bathy_elevation_check (p,
        args.bathy_min_elevation);

    // Bathy photons can't be above the sea surface
    p = detail::relative_depth_check (p, args.water_column_width);

    // Sea surface photons must all be near the elevation estimate
    p = detail::surface_range_check (p, args.surface_range);

    // Bathy photons must all be near the elevation estimate
    p = detail::bathy_range_check (p, args.bathy_range);

    return p;
}

} // namespace ATL24_coastnet
