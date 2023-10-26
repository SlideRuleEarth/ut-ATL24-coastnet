#include "ATL24_resnet/utils.h"
#include "viper/geotiff.h"
#include "viper/raster.h"
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <unordered_set>

using namespace std;
using namespace viper::geotiff;
using namespace viper::raster;
using namespace ATL24_resnet;

template<typename T>
geotiff_raster<T> read (const string &fn)
{
    // Turn off errors and warnings
    CPLErrorHandlerSetter cpl_error_handler_setter;

    // Init GDAL
    GDALAllRegister ();

    RAII_wrapper wrapper (fn.c_str (), GA_ReadOnly);
    GDALDataset *data_set = wrapper.get_pointer();
    if (data_set == nullptr)
        throw runtime_error ("Can't read GeoTIFF file: " + fn);

    // Get projection information in wkt
    char *wkt_ptr = nullptr;
    string projection = "";
    // If there is an SRS available, get the WKT1_GDAL interpretation of it.
    if(data_set->GetSpatialRef() != NULL)
    {
        const OGRSpatialReference *oSRS = data_set->GetSpatialRef();
        const char *apszOptions[] = {"FORMAT=WKT1_GDAL", "MULTILINE=NO", nullptr};
        oSRS->exportToWkt(&wkt_ptr, apszOptions);
        projection = string(wkt_ptr);
    }

    // Get GeoTransform
    double g[6];
    if (data_set->GetGeoTransform (g) != CE_None)
        throw runtime_error ("Can't read GeoTransform from GeoTIFF file: " + fn);

    // Get all bands
    vector<int> band_ids;

    // Determine how many there are
    int count = data_set->GetRasterCount ();

    // Stuff the ids into 'band_ids'
    band_ids.resize (count);

    // Band ids are one-based
    iota (band_ids.begin (), band_ids.end (), 1);

    // Create the raster struct
    geotiff_raster<T> raster;
    raster.wkt = projection;
    raster.min_x = g[0];
    raster.max_y = g[3];
    raster.resolution_x = g[1];
    raster.resolution_y = -g[5];

    // For each band id
    for (auto band_id : band_ids)
    {
        // Read raster band
        GDALRasterBand *band = data_set->GetRasterBand (band_id);

        if (band == nullptr)
            throw runtime_error ("Can't get GeoTIFF raster band");

        const int xsize = band->GetXSize();
        const int ysize = band->GetYSize();

        // Alloc buffer
        vector<float> data (xsize * ysize);

        // Get the data type
        const auto data_type = band->GetRasterDataType();

        switch (data_type)
        {
            default:
            throw runtime_error ("Unsupported GeoTIFF raster data type. Expected a 32bit float");

            case GDT_Float32:
            {
                // Do the read
                if (band->RasterIO (GF_Read,
                    0, 0,
                    xsize, ysize,
                    &data[0],
                    xsize, ysize, GDT_Float32, 0, 0) != CE_None)
                    throw runtime_error ("Can't read GeoTIFF raster band");
            }
            break;
        }

        // Get no-data value
        int has_no_data = 0;
        auto no_data_value = band->GetNoDataValue(&has_no_data);

        // Copy band (and transform any non-data values to our non-data value).
        T image (ysize, xsize); // rows, cols
        assert(data.size() == image.size());
        if (has_no_data && no_data_value != UNDEFINED)
        {
            transform(data.begin(), data.end(), image.begin(),
                           [=](double value) { return (value==no_data_value) ? UNDEFINED : value; });
        }
        else
        {
            copy (data.begin (), data.end (), image.begin ());
        }

        // Save the band image
        raster.bands.push_back (image);
    }

    return raster;
}

// See: https://www.eye4software.com/hydromagic/documentation/manual/utilities/egm2008-geoid/
class egm2008
{
    public:
    egm2008 ()
    {
        clog << "Reading " << filename << endl;
        const auto g = ::read<raster<float>> (filename);

        if (g.bands.empty ())
            throw runtime_error ("No raster bands were read");

        // Show the properties of the geotiff
        clog << "Raster_bands " << g.bands.size () << endl;
        clog << "Raster_band_size " << g.bands[0].rows ()
            << " X " << g.bands[0].cols () << endl;
        clog << "Spatial_reference_system '" << g.wkt << "'" << endl;
        clog << "Resolution x " << g.resolution_x << endl;
        clog << "Resolution y " << g.resolution_y << endl;
        clog << "Top_left " << g.min_x << ", " << g.max_y << endl;

        // Check against hard-coded values
        if (g.bands.size () != 1)
            throw runtime_error ("Expected 1 raster band");
        if (g.bands[0].rows () != rows)
            throw runtime_error (string("Expected ")
                + to_string(rows)
                + " raster band rows");
        if (g.bands[0].cols () != cols)
            throw runtime_error (string("Expected ")
                + to_string(cols)
                + " raster band columns");
        if (static_cast<size_t> (round (g.resolution_x * precision))
            != static_cast<size_t> (round (resolution * precision)))
            throw runtime_error (string("Expected X resolution to be about ")
                + to_string(static_cast<double> (resolution) / precision));
        if (static_cast<size_t> (round (g.resolution_y * precision))
            != static_cast<size_t> (round (resolution * precision)))
            throw runtime_error (string("Expected Y resolution to be about ")
                + to_string(static_cast<double> (resolution) / precision));
        if (static_cast<int> (round (g.min_x)) != top_left_x)
            throw runtime_error (string ("Expected the top left X to be about ")
                + to_string (top_left_x));
        if (static_cast<int> (round (g.max_y)) != top_left_y)
            throw runtime_error (string ("Expected the top left Y to be about ")
                + to_string (top_left_y));

        // Save the corrections
        r = g.bands[0];
        clog << "Correction model saved as "
            << r.rows () << " X " << r.cols ()
            << " minute raster" << endl;
    }
    double to_msl (double lat, double lon, const double z) const
    {
        // Subtract off the top left offset
        lon -= top_left_x;
        lat = top_left_y - lat;

        // Convert lat/lon to minutes
        lat /= resolution;
        lon /= resolution;

        // Center it on a minute
        lat -= resolution / 2.0;
        lon -= resolution / 2.0;

        // Get the raster indexes
        const size_t i = round (lat);
        const size_t j = round (lon);

        // Get the geoid elevation
        assert (i < r.rows ());
        assert (j < r.cols ());
        const double g = r (i, j);

        // Do the correction
        return z - g;
    }

    private:
    const string filename = string("us_nga_egm2008_1.tif");
    static constexpr size_t rows = 10801;
    static constexpr size_t cols = 21601;
    static constexpr size_t precision = 100000;
    static constexpr double resolution = 1.0 / 60.0; // degrees
    static constexpr int top_left_x = -180;
    static constexpr int top_left_y = 90;
    raster<float> r;
};

vector<classified_point3d> read_lat_lon (istream &is)
{
    string line;

    // Get the column labels
    getline (cin, line);

    clog << "Column labels: " << line << endl;

    vector<classified_point3d> p;
    while (getline (cin, line))
    {
        stringstream ss (line);
        size_t h5_index;
        ss >> h5_index;
        ss.get (); // ignore ','
        double ts, lat, lon, z;
        ss >> ts;
        ss.get (); // ignore ','
        ss >> lat;
        ss.get (); // ignore ','
        ss >> lon;
        ss.get (); // ignore ','
        ss >> z;
        size_t cls;
        ss.get (); // ignore ','
        ss >> cls;
        p.push_back (classified_point3d {h5_index, ts, lat, lon, z, cls});
    }

    return p;
}

template<typename T,typename U>
T wgs84_to_msl (T p, const U &egm)
{
    // Convert elevation at lat(x), lon(y) to msl
#pragma omp parallel for
    for (size_t i = 0; i < p.size (); ++i)
        p[i].z = egm.to_msl (p[i].lat, p[i].lon, p[i].z);

    return p;
}

// https://en.wikipedia.org/wiki/Haversine_formula
template<typename T>
double haversine_distance (const T &p1, const T &p2)
{
    const double lat1 =  p1.lat * M_PI / 180.0; // convert to radians
    const double lat2 =  p2.lat * M_PI / 180.0; // convert to radians
    const double lon1 =  p1.lon * M_PI / 180.0; // convert to radians
    const double lon2 =  p2.lon * M_PI / 180.0; // convert to radians
    const double dlat =  lat2 - lat1;
    const double dlon =  lon2 - lon1;
    const double earth_radius_m = 6'378'137;
    const double h1 = sin (dlat / 2.0) * sin (dlat / 2.0);
    const double h2 = cos (lat1) * cos (lat2) * sin (dlon / 2.0) * sin (dlon / 2.0);
    const double d = 2.0 * earth_radius_m * asin (sqrt (h1 + h2));
    return d;
}

template<typename T>
vector<classified_point2d> convert_to_meters (T p)
{
    // Sort by latitude
    sort (p.begin (), p.end (),
        [](const classified_point3d &a, const classified_point3d &b)
        { return a.lat < b.lat; });

    const auto p0 = p[0];
    vector<classified_point2d> xy (p.size ());
#pragma omp parallel for
    for (size_t i = 0; i < p.size (); ++i)
    {
        const auto d = haversine_distance (p0, p[i]);
        xy[i] = classified_point2d {p[i].h5_index, d, p[i].z, p[i].cls};
    }

    // Sort by X
    sort (xy.begin (), xy.end (),
        [](const classified_point2d &a, const classified_point2d &b)
        { return a.x < b.x; });

    return xy;
}

template<typename T,typename U>
double get_mean (const T &w, const U &p)
{
    return accumulate (w.begin (), w.end (), 0.0,
        [&] (const double a, const double b)
        { return a + p[b].y;}) / w.size ();
}

template<typename T,typename U>
double get_std (const T &w, const U &p)
{
    // Get E[X]
    const auto m = get_mean (w, p);

    // Get E[X^2]
    const auto ex2 = accumulate (w.begin (), w.end (), 0.0,
        [&] (const double a, const double b)
        { return a + p[b].y * p[b].y; }) / w.size ();

    // Var(X) = E[X^2] - E[X]^2
    return sqrt (ex2 - m * m);
}

struct ws_params
{
    double min_elevation = -1.0;
    double max_elevation = 1.0;
    double window_size = 1000; // meters
    size_t min_points = 300;
};

template<typename T,typename U,typename V>
bool is_water_surface (const size_t i, const T &p, const U &w, const V &params)
{
    // Check params
    assert(i < p.size ());

    // Is it too deep?
    if (p[i].y < params.min_elevation)
        return false;

    // Is it too high?
    if (p[i].y > params.max_elevation)
        return false;

    // Do we have enough points?
    if (w.size () < params.min_points)
        return false;

    // Get the indexes of the points near sea level
    vector<size_t> nsl;
    nsl.reserve (w.size ());

    for (auto it = w.begin (); it != w.end (); ++it)
    {
        // Check bounds
        if ((p[*it].y < params.min_elevation)
            || (p[*it].y > params.max_elevation))
            continue;
        // Save it
        nsl.push_back (*it);
    }

    // Half of them should be near sea level
    if (nsl.size () < w.size () / 2)
        return false;

    // Compute the mean and stddev
    //
    // Is the point within a certain confidence interval?
    const auto m = get_mean (nsl, p);
    const auto s = get_std (nsl, p);
    const auto d = fabs (p[i].y - m);
    if (d > s * 2.5)
        return false;

    // Yes, it's on the water surface
    return true;
}

// Determine which points are water surface
//
// Assumes that the input is sorted by X value
template<typename T,typename U>
vector<char> get_ws_flags (const T &p, const U &params)
{
    vector<char> ws_flags (p.size ());

    // 'i' is the start of the window, inclusive
    size_t i = 0;

    // 'j' is the end of the window, non-inclusive
    size_t j = 0;

    // Point indexes sorted by elevation
    auto f = [&](const size_t a, const size_t b)
        { return p[a].y < p[b].y;};
    multiset<size_t,decltype(f)> w (f);

    // For each starting point
    while (i < p.size ())
    {
        // Open the right side of the window
        double window_size = fabs (p[j].x - p[i].x);
        while (window_size < params.window_size && j < p.size ())
        {
            // Add point at 'j' to the window
            w.insert (j);

            // Recompute the size
            window_size = fabs (p[j].x - p[i].x);

            // 'j' is non-inclusive
            ++j;
        }

        // Are we done?
        if (j == p.size ())
            break;

        // Is 'i' on the water surface?
        if (is_water_surface (i, p, w, params))
            ws_flags[i] = true;

        // Close the left side of the window
        w.erase (i);

        // 'i' is inclusive
        ++i;
    }

    return ws_flags;
}

int main (int argc, char **argv)
{
    try
    {
        if (argc != 1)
            throw runtime_error ("Usage: pgm < filename.txt");

        // Initialize MSL correction
        clog << "Initializing EGM 2008 model" << endl;
        egm2008 egm;

        // Read the points
        auto lat_lon = read_lat_lon (cin);
        clog << lat_lon.size () << " points read" << endl;

        // Convert from WGS84 to MSL
        clog << "Converting to MSL" << endl;
        lat_lon = wgs84_to_msl (lat_lon, egm);

        // Convert to:
        //     x = distance in meters along beam path
        //     y = elevation
        clog << "Converting to along-track meters" << endl;
        auto xy = convert_to_meters (lat_lon);

        // Output to terminal
        write_classified_point2d (cout, xy);

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
