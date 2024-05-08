#ifndef GEOTIFF_H
#define GEOTIFF_H

#include "raster.h"
#include "container_utils.h"
#include <gdal/gdal_priv.h>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <numeric>

namespace ATL24_coastnet
{

namespace geotiff
{

//A wrapper class for GDAL objects to make them RAII compliant
class RAII_wrapper{

GDALDataset *data_set;

public:
    RAII_wrapper(GDALDriver *driver, const char *file_name, int raster_width, int raster_height, int number_bands,
            GDALDataType data_type, char **options)
    {
        data_set = driver->Create(file_name, raster_width, raster_height, number_bands, data_type, options);
    }

    RAII_wrapper(const char *file_name, GDALAccess access)
    {
        data_set = (GDALDataset *) GDALOpen (file_name, access);
    }

    ~RAII_wrapper(void)
    {
        GDALClose ((GDALDatasetH) data_set);
    }

    GDALDataset* get_pointer(void)
    {
        return data_set;
    }

};

/// This value comes from the GeoTiff specification.
const double UNDEFINED = -9999.0f;

// The resolution value is the resolution in the x direction
template<typename T>
struct geotiff_raster
{
    std::string wkt;
    double min_x;
    double max_y;
    double resolution_x;
    double resolution_y;
    std::vector<T> bands;

    // Operators
    bool operator== (const geotiff_raster &other) const
    {
        return (wkt == other.wkt)
            && (min_x == other.min_x)
            && (max_y == other.max_y)
            && (resolution_x == other.resolution_x)
            && (resolution_y == other.resolution_y)
            && (bands == other.bands);
    }
    bool operator!= (const geotiff_raster &other) const
    {
        return !(*this == other);
    }
};

/// @brief Write raster bands to a geotiff file
/// @tparam T band type
/// @param raster Geotiff raster
/// @param no_data_value The value of pixels that have no data
/// @param has_no_data_value The value of pixels contain no_data values
/// @throw runtime_error
template<typename T>
void write (const geotiff_raster<T> &raster,
    const std::string &fn,
    const GDALDataType data_type = GDT_Float64,
    const double no_data_value = UNDEFINED,
    const bool has_no_data_value = true)
{
    // Check the raster
    if (raster.bands.empty ())
        throw std::runtime_error ("The raster bands are empty");

    // They must all be the same size
    for (size_t i = 1; i < raster.bands.size (); ++i)
        if (raster.bands[0].size () != raster.bands[i].size ())
            throw std::runtime_error ("The raster bands are not the same size");

    // Init GDAL
    GDALAllRegister ();

    // Get GDAL GTiff driver
    const std::string name ("GTiff");
    GDALDriver *gdal_driver = GetGDALDriverManager()->GetDriverByName (name.c_str ());

    // GCOV_EXCL_START
    if (!gdal_driver)
        throw std::runtime_error ("Can't get GDAL driver: " + name);
    // GCOV_EXCL_STOP

    // GCOV_EXCL_START
    // Make sure it supports the Create() method
    if (!CSLFetchBoolean (gdal_driver->GetMetadata(), GDAL_DCAP_CREATE, FALSE))
        throw std::runtime_error ("Driver does not support the 'Create()' method: " + name);
    // GCOV_EXCL_STOP

    // Allocate the dataset for the raster bands
    assert (!raster.bands.empty ());
    RAII_wrapper wrapper (gdal_driver,
            fn.c_str (),
            raster.bands[0].cols (),
            raster.bands[0].rows (),
            raster.bands.size (), data_type, nullptr);
    GDALDataset *data_set = wrapper.get_pointer();

    // GCOV_EXCL_START
    if (!data_set)
        throw std::runtime_error ("Can't open GeoTIFF file for writing: " + fn);
    // GCOV_EXCL_STOP

    // GCOV_EXCL_START
    
    /* Set the projection. If the raster's WKT is empty, no SRS is available for this region.
    Don't throw an error in this case - just continue.*/
    if(!raster.wkt.empty())
    {
        OGRSpatialReference oSRS(raster.wkt.c_str());
        if(data_set->SetSpatialRef(&oSRS) != CE_None)
            throw std::runtime_error ("Can't set GeoTIFF projection: " + raster.wkt);
    }
    // GCOV_EXCL_STOP

    // Set the affine transform.
    //
    // From the GDAL docs: "In a north up image, p[1] is the pixel width,
    // and p[5] is the pixel height. The upper left corner of the upper
    // left pixel is at position (p[0], p[3])."
    double p[6] = {raster.min_x, raster.resolution_x, 0, raster.max_y, 0, -raster.resolution_y};
    // GCOV_EXCL_START
    if (data_set->SetGeoTransform (p) != CE_None)
        throw std::runtime_error ("Can't set GeoTIFF GeoTransform");
    // GCOV_EXCL_STOP

    // Copy the raster bands to the buffer
    for (size_t band_num = 0; band_num < raster.bands.size (); ++band_num)
    {
        // Allocate the raster band buffer and copy data to it because the
        // RasterIO function does not take a const pointer.
        std::vector<typename T::value_type> tmp (raster.bands[band_num].begin (), raster.bands[band_num].end ());

        // Get a pointer to the raster.
        //
        // Band IDs are 1-based.
        GDALRasterBand *band = data_set->GetRasterBand (band_num + 1);

        // GCOV_EXCL_START
        if (band == nullptr)
            throw std::runtime_error ("Can't get GeoTIFF raster band");
        // GCOV_EXCL_STOP

        // Set no_data value, if specified
        if (has_no_data_value)
            band->SetNoDataValue(no_data_value);

        // Write data to the raster
        // GCOV_EXCL_START
        if (band->RasterIO (GF_Write,
            0, 0,
            raster.bands[band_num].cols (), raster.bands[band_num].rows (),
            &tmp[0],
            raster.bands[band_num].cols (), raster.bands[band_num].rows (),
            data_type,
            0, 0) != CE_None)
            throw std::runtime_error ("Can't write GeoTIFF raster band");
        // GCOV_EXCL_STOP
    }

}

/// @brief Write a raster to a geotiff file
/// @tparam T band type
/// @param band The raster band
/// @param fn The raster filename
/// @param wkt The OGC "Well Known Text", or WKT, describing the Spatial Reference System
/// @param min_x The x offset of the top left of the image
/// @param max_y The y offset of the top left of the image
/// @param resolution The resolution in meters per pixel (Note that this function assumes
///     resolution in x and y is the same)
/// @param no_data_value The value of pixels that have no data
/// @throw runtime_error
void write_band (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution,
    const double no_data_value,
    const bool has_no_data_value)
{
    using T = ATL24_coastnet::raster::raster<double>;
    geotiff_raster<T> r {wkt,
                         min_x,
                         max_y,
                         resolution,
                         resolution,
                         std::vector<T> (1, band)};
    write (r, fn, GDT_Float64, no_data_value, has_no_data_value);
}

/// @brief Write a raster to a geotiff file
/// @tparam T band type
/// @param band The raster band
/// @param fn The raster filename
/// @param wkt The OGC "Well Known Text", or WKT, describing the Spatial Reference System
/// @param min_x The x offset of the top left of the image
/// @param max_y The y offset of the top left of the image
/// @param resolution_x The resolution in meters per pixel in the x direction
/// @param resolution_y The resolution in meters per pixel in the y direction
/// @param no_data_value The value of pixels that have no data
/// @throw runtime_error
void write_band (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution_x,
    const double resolution_y,
    const double no_data_value,
    const bool has_no_data_value)
{
    using T = ATL24_coastnet::raster::raster<double>;
    geotiff_raster<T> r {wkt,
                         min_x,
                         max_y,
                         resolution_x,
                         resolution_y,
                         std::vector<T> (1, band)};
    write (r, fn, GDT_Float64, no_data_value, has_no_data_value);
}

// Write an 8 bit geotiff
void write_8bit_to_8bit (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution_x,
    const double resolution_y)
{
    // Force the values into the range [1,255]
    //
    // UNDEFINED values will get converted to 0's
    auto tmp (band);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(),
        [=](double value) {
            if (value == UNDEFINED)
                return 0.0;
            return std::max (std::min (value, 255.0), 1.0);
        });
    using T = ATL24_coastnet::raster::raster<uint8_t>;
    geotiff_raster<T> r {wkt,
                         min_x,
                         max_y,
                         resolution_x,
                         resolution_y,
                         std::vector<T> (1, tmp)};
    const bool has_no_data_value = true;
    write (r, fn, GDT_Byte, 0.0, has_no_data_value);
}

// Write an 8bit geotiff with x and y resolution the same
void write_8bit_to_8bit (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution)
{
    write_8bit_to_8bit(band,
            fn,
            wkt,
            min_x,
            max_y,
            resolution,
            resolution);
}


/// A 32-bit integer band is assumed to contain ABGR packed pixels, so it
/// will get split into 3 separate RGB bands.
void write_32bit_to_8bit (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution_x,
    const double resolution_y)
{
    using T = ATL24_coastnet::raster::raster<uint8_t>;
    T red (band.rows (), band.cols ());
    T green (band.rows (), band.cols ());
    T blue (band.rows (), band.cols ());
    // Split RGBA values
    for (size_t i = 0; i < band.size (); ++i)
    {
        assert (i < red.size ());
        assert (i < green.size ());
        assert (i < blue.size ());
        if (band[i] != UNDEFINED)
        {
            // Force the values into the range [1,255]
            red[i] = (static_cast<int> (band[i]) >> 0) & 0xFF;
            green[i] = (static_cast<int> (band[i]) >> 8) & 0xFF;
            blue[i] = (static_cast<int> (band[i]) >> 16) & 0xFF;
            red[i] = std::max (red[i], uint8_t (1));
            green[i] = std::max (green[i], uint8_t (1));
            blue[i] = std::max (blue[i], uint8_t (1));
        }
        else
        {
            // UNDEFINED values will get converted to 0's
            red[i] = 0;
            green[i] = 0;
            blue[i] = 0;
        }
    }
    std::vector<T> bands {red, green, blue};

    geotiff_raster<T> r {wkt,
                         min_x,
                         max_y,
                         resolution_x,
                         resolution_y,
                         bands};

    // Write 8-bit
    const bool has_no_data_value = true;
    write (r, fn, GDT_Byte, 0.0, has_no_data_value);
}

/// A 32-bit integer band is assumed to contain ABGR packed pixels, so it
/// will get split into 3 separate RGB bands. Assumes x resolution and
/// y resolution are the same
void write_32bit_to_8bit (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution)
{
    write_32bit_to_8bit(band,
            fn,
            wkt,
            min_x,
            max_y,
            resolution,
            resolution);
}

/// A 32-bit integer band is assumed to contain ABGR packed pixels, so it
/// will get split into 4 separate RGBA bands.
void write_32bit_to_double (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution_x,
    const double resolution_y)
{
    using T = ATL24_coastnet::raster::raster<double>;
    T red (band.rows (), band.cols ());
    T green (band.rows (), band.cols ());
    T blue (band.rows (), band.cols ());
    T alpha (band.rows (), band.cols ());
    // Split RGBA values
    for (size_t i = 0; i < band.size (); ++i)
    {
        assert (i < red.size ());
        assert (i < green.size ());
        assert (i < blue.size ());
        assert (i < alpha.size ());
        if (band[i] != UNDEFINED)
        {
            // Force the values into the range [0,255]
            red[i] = (static_cast<int> (band[i]) >> 0) & 0xFF;
            green[i] = (static_cast<int> (band[i]) >> 8) & 0xFF;
            blue[i] = (static_cast<int> (band[i]) >> 16) & 0xFF;
            alpha[i] = 0xFF;
        }
        else
        {
            red[i] = UNDEFINED;
            green[i] = UNDEFINED;
            blue[i] = UNDEFINED;
            alpha[i] = UNDEFINED;
        }
    }
    std::vector<T> bands {red, green, blue, alpha};
    geotiff_raster<T> r {wkt,
                         min_x,
                         max_y,
                         resolution_x,
                         resolution_y,
                         bands};
    // Write doubles
    const bool has_no_data_value = true;
    write (r, fn, GDT_Float64, 0.0, has_no_data_value);
}

/// A 32-bit integer band is assumed to contain ABGR packed pixels, so it
/// will get split into 4 separate RGBA bands. Assumes x and y resolution are
/// the same
void write_32bit_to_double (const ATL24_coastnet::raster::raster<double> &band,
    const std::string &fn,
    const std::string &wkt,
    const double min_x,
    const double max_y,
    const double resolution)
{
    write_32bit_to_double(band,
            fn,
            wkt,
            min_x,
            max_y,
            resolution,
            resolution);
}

/// @brief Helper object for setting/restoring CPL error handler
class CPLErrorHandlerSetter
{
    private:
    CPLErrorHandler old_cpl_error_handler;

    public:
    CPLErrorHandlerSetter ()
    {
        // Set the new handler and save the old one
        old_cpl_error_handler = CPLSetErrorHandler (CPLQuietErrorHandler);
    }
    ~CPLErrorHandlerSetter ()
    {
        // Restore the old handler
        CPLSetErrorHandler (old_cpl_error_handler);
    }
};

/// @brief Read geotiff raster from a geotiff file
/// @tparam T band type
/// @param fn The raster filename
/// @return A geotiff raster object
/// @throw runtime_error
template<typename T>
geotiff_raster<T> read (const std::string &fn)
{
    // Currently this only supports reading doubles
    static_assert (std::is_same<typename T::value_type,double>::value, "geotiff_raster<T>read() only supports doubles");

    // Turn off errors and warnings
    CPLErrorHandlerSetter cpl_error_handler_setter;

    // Init GDAL
    GDALAllRegister ();

    RAII_wrapper wrapper (fn.c_str (), GA_ReadOnly);
    GDALDataset *data_set = wrapper.get_pointer();

    // GCOV_EXCL_START
    if (data_set == nullptr)
        throw std::runtime_error ("Can't read GeoTIFF file: " + fn);
    // GCOV_EXCL_STOP

    // Get projection information in wkt
    char *wkt_ptr = nullptr;
    std::string projection = "";
    // If there is an SRS available, get the WKT1_GDAL interpretation of it.
    if(data_set->GetSpatialRef() != NULL)
    {
        const OGRSpatialReference *oSRS = data_set->GetSpatialRef();
        const char *apszOptions[] = {"FORMAT=WKT1_GDAL", "MULTILINE=NO", nullptr};
        oSRS->exportToWkt(&wkt_ptr, apszOptions);
        projection = std::string(wkt_ptr);
    }

    // Get GeoTransform
    double g[6];
    // GCOV_EXCL_START
    if (data_set->GetGeoTransform (g) != CE_None)
        throw std::runtime_error ("Can't read GeoTransform from GeoTIFF file: " + fn);
    // GCOV_EXCL_STOP

    // Get all bands
    std::vector<int> band_ids;

    // Determine how many there are
    int count = data_set->GetRasterCount ();

    // Stuff the ids into 'band_ids'
    band_ids.resize (count);

    // Band ids are one-based
    std::iota (band_ids.begin (), band_ids.end (), 1);

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

        // GCOV_EXCL_START
        if (band == nullptr)
            throw std::runtime_error ("Can't get GeoTIFF raster band");
        // GCOV_EXCL_STOP

        const int xsize = band->GetXSize();
        const int ysize = band->GetYSize();

        // Alloc buffer
        std::vector<double> data (xsize * ysize);

        // Get the data type
        const auto data_type = band->GetRasterDataType();

        switch (data_type)
        {
            // GCOV_EXCL_START
            default:
            throw std::runtime_error ("Unknown GeoTIFF raster data type");
            // GCOV_EXCL_STOP

            case GDT_Float64:
            {
                // Do the read
                // GCOV_EXCL_START
                if (band->RasterIO (GF_Read,
                    0, 0,
                    xsize, ysize,
                    &data[0],
                    xsize, ysize, GDT_Float64, 0, 0) != CE_None)
                    throw std::runtime_error ("Can't read GeoTIFF raster band");
                // GCOV_EXCL_STOP
            }
            break;

            case GDT_Float32:
            {
                // First read the 32-bit data
                std::vector<uint32_t> tmp (xsize * ysize);
                // GCOV_EXCL_START
                if (band->RasterIO (GF_Read,
                    0, 0,
                    xsize, ysize,
                    &tmp[0],
                    xsize, ysize, GDT_Float32, 0, 0) != CE_None)
                    throw std::runtime_error ("Can't read GeoTIFF raster band");
                // GCOV_EXCL_STOP
                // Convert it to double
                data = std::vector<double> (tmp.begin (), tmp.end ());
            }
            break;

            case GDT_Byte:
            {
                // First read the 8-bit data
                std::vector<uint8_t> tmp (xsize * ysize);
                // GCOV_EXCL_START
                if (band->RasterIO (GF_Read,
                    0, 0,
                    xsize, ysize,
                    &tmp[0],
                    xsize, ysize, GDT_Byte, 0, 0) != CE_None)
                    throw std::runtime_error ("Can't read GeoTIFF raster band");
                // GCOV_EXCL_STOP
                // Convert it to double
                data = std::vector<double> (tmp.begin (), tmp.end ());
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
            std::transform(data.begin(), data.end(), image.begin(),
                           [=](double value) { return ((value==no_data_value)||(isnan(value)))
                                                       ? UNDEFINED : value; });
        }
        else
        {
            std::copy (data.begin (), data.end (), image.begin ());
        }
        assert(!container_utils::contains_nan(image));

        // Save the band image
        raster.bands.push_back (image);
    }

    return raster;
}

/// @brief Helper function for reading a geotiff
/// @param verbose Print verbose output
/// @param fn The geotiff filename
/// @param os Optional opput stream specifier
/// @return A geotiff raster object
/// @throw runtime_error
ATL24_coastnet::geotiff::geotiff_raster<ATL24_coastnet::raster::raster<double>> read (
    const bool verbose,
    const std::string &fn,
    std::ostream &os = std::clog)
{
    // GCOV_EXCL_START
    if (verbose)
        os << "Opening " << fn << std::endl;
    // GCOV_EXCL_STOP

    // Make sure it exists
    std::ifstream ifs (fn);

    // GCOV_EXCL_START
    if (!ifs)
        throw std::runtime_error ("Cannot open input file for reading");
    // GCOV_EXCL_STOP

    // Read the GEOTIFF raster
    auto r = read<raster::raster<double>> (fn);

    // GCOV_EXCL_START
    if (r.bands.empty ())
        throw std::runtime_error ("No raster bands were read");
    // GCOV_EXCL_STOP

    // GCOV_EXCL_START
    if (verbose)
    {
        os << "\tRaster_bands " << r.bands.size () << std::endl;
        os << "\tRaster_band_size " << r.bands[0].cols () << " X " << r.bands[0].rows () << std::endl;
        os << "\tSpatial_reference_system '" << r.wkt << "'" << std::endl;
        os << "\tResolution x " << r.resolution_x << std::endl;
        os << "\tResolution y " << r.resolution_y << std::endl;
        os << "\tTop_left " << r.min_x << ", " << r.max_y << std::endl;
    }
    // GCOV_EXCL_STOP

    // Return the raster
    return r;
}

} // namespace geotiff

} // namespace ATL24_coastnet

#endif // GEOTIFF_H
