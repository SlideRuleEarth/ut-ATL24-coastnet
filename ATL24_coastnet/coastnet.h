#pragma once

#include <iostream>
#include <unordered_map>

namespace ATL24_coastnet
{

constexpr double surface_min_elevation = -20.0;
constexpr double surface_max_elevation = 20.0;

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


} // namespace coastnet
