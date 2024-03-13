#pragma once

#include "ATL24_coastnet/coastnet.h"
#include <vector>

namespace ATL24_coastnet
{

template<typename T,typename U>
T blunder_detection (T p, const U &args)
{
    using namespace std;

    // Reclassify photons using heuristics
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

    // If there's no bathy or surface, there's nothing to do
    if (total_surface == 0 && total_bathy == 0)
        return p;

    // Surface photons must be near sea level
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-surface
        if (p[i].cls != sea_surface_class)
            continue;

        // Too high?
        if (p[i].z > args.surface_max_elevation)
            p[i].cls = 0;

        // Too low?
        if (p[i].z < args.surface_min_elevation)
            p[i].cls = 0;
    }

    if (total_bathy == 0)
        return p;

    // Bathy photons can't be too deep
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].cls != bathy_class)
            continue;

        // Too deep?
        if (p[i].z < args.bathy_min_elevation)
            p[i].cls = 0;
    }

    if (total_surface == 0)
        return p;

    // Bathy can only be 'under' the surface if there is a surface photon nearby
    const auto nearby_surface_indexes = get_nearest_along_track_prediction (p, 41);

    // Bathy photons can't be above the sea surface
    for (size_t i = 0; i < p.size (); ++i)
    {
        // Ignore non-bathy
        if (p[i].cls != bathy_class)
            continue;

        // Get the closest surface index
        const size_t j = nearby_surface_indexes[i];

        // How far away is it?
        assert (j < p.size ());
        const double dx = fabs (p[i].x - p[j].x);

        // If it's too far away, we can't do the check
        if (dx > args.water_column_width)
            continue;

        // If the bathy below the surface?
        if (p[i].z < p[j].z)
            continue; // Yes, keep going

        // No, reassign
        p[i].cls = 0;
    }

    return p;
}

} // namespace ATL24_coastnet
