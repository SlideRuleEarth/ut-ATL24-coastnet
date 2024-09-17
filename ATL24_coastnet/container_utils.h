#ifndef CONTAINER_UTILS_H
#define CONTAINER_UTILS_H

#include "precompiled.h"

namespace ATL24_coastnet
{

namespace container_utils
{

/// @brief Normalize a container to 0.0, 1.0
/// @tparam T Container type
/// @param x Container
/// @return Normalized values
template <typename T>
T normalize (const T &x)
{
    assert (!x.empty ());
    const auto xmin = *std::min_element (x.begin (), x.end ());
    const auto xmax = *std::max_element (x.begin (), x.end ());
    T y (x);

    // Do the scaling
    const double d = xmax - xmin;
    for (size_t i = 0; i < y.size (); ++i)
        y[i] = (y[i] - xmin) / d;

    return y;
}

/// @brief Get the mean of the values in a container
/// @tparam T Container type
/// @tparam U Mask type
/// @param x Container
/// @param mask The mask
/// @return Mean
template <typename T, typename U>
double mean (const T &x, const U &mask)
{
    // Check args
    assert (mask.empty () || x.size () == mask.size ());

    size_t total = 0;
    double sum = 0.0;
    for (size_t i = 0; i < x.size (); ++i)
    {
        // Skip 0's in the mask
        if (!mask.empty () && !mask[i])
            continue;

        ++total;
        sum += x[i];
    }

    // Degenerate case
    if (total == 0)
        return 0.0;

    return sum / total;
}

/// @brief Unmasked version of mean()
template <typename T>
double mean (const T &x)
{
    // Empty mask
    T mask;
    return mean (x, mask);
}

/// @brief Get the variance of the values in a container
/// @tparam T Container type
/// @tparam U Mask type
/// @param x Container
/// @param mask The mask
/// @return Variance
template <typename T, typename U>
double variance (const T &x, const U &mask)
{
    // Check args
    assert (mask.empty () || x.size () == mask.size ());

    size_t total = 0;
    double sum = 0.0;
    double sum2 = 0.0;
    for (size_t i = 0; i < x.size (); ++i)
    {
        // Skip 0's in the mask
        if (!mask.empty () && !mask[i])
            continue;

        ++total;
        sum += x[i];
        sum2 += x[i] * x[i];
    }

    // Degenerate case
    if (total == 0)
        return 0.0;

    // Variance = E[x^2] - E[x]^2
    const double mean = sum / total;
    const double var = sum2 / total - mean * mean;

    // E[x^2] >= E[x]^2
    assert (var >= 0.0);

    return var;
}

/// @brief Unmasked version of variance()
template <typename T>
double variance (const T &x)
{
    // Empty mask
    T mask;
    return variance (x, mask);
}

/// @brief Get the z-scores of the values in a container
/// @tparam T Container type
/// @tparam U Mask type
/// @param x Container
/// @param mask The mask
/// @return Z-scored values
template <typename T, typename U>
T z_score (const T &x, const U &mask)
{
    // Check args
    assert (mask.empty () || x.size () == mask.size ());

    const double u = mean (x);
    const double s = std::sqrt (variance (x));

    // Copy it
    T y (x);

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < y.size (); ++i)
        y[i] = (y[i] - u) / s;

    return y;
}

/// @brief Unmasked version of z_score()
template <typename T>
T z_score (const T &x)
{
    // Empty mask
    T mask;
    return z_score (x, mask);
}

/// @brief Get the median value from an unsorted container
/// @tparam T Container type
/// @param x Unsorted container
/// @return The median
///
/// This function rearranges the values in 'x'.
template <typename T>
typename T::value_type median (T &x)
{
    // Check args
    assert (!x.empty ());

    // Partial sort is O(n) vs O(n lg (n)) for sort()
    std::nth_element (x.begin (), x.begin () + x.size () / 2, x.end ());

    // Get the median
    return x[x.size () / 2];
}

/// @brief Median 2D filter a raster
/// @tparam T Raster type
/// @tparam U Mask type
/// @param x The raster to filter
/// @param sz The size of the filter kernel
/// @param mask The mask
/// @return Filtered container
///
/// Corresponding values in the mask that are 0 are skipped in the raster
///
/// The meaning of the 'sz' parameter is as follows:
///
///     sz=1 -> 3x3 filter
///     sz=2 -> 5x5 filter
///     sz=3 -> 7x7 filter
///     ..., etc.
template <typename T, typename U>
T median_2D_filter (const T &x, const size_t sz, const U &mask)
{
    // Check args
    assert (mask.empty () || x.size () == mask.size ());

    // Return value
    T r (x);

    // Make sure it's big enough
    if (x.rows () < sz + 1 || x.cols () < sz + 1)
        return r;

    // Required for parallel for
    const size_t max_rows = x.rows () - sz;

#pragma omp parallel for
    for (size_t i = sz; i < max_rows; ++i)
    {
        for (size_t j = sz; j + sz < x.cols (); ++j)
        {
            if (!mask.empty () && !mask (i, j))
                continue;

            // 'tmp' will hold the values of the square filter
            //
            // sz=1 -> 3x3 = 9 tap
            // sz=2 -> 5x5 = 25 tap
            // sz=3 -> 7x7 = 49 tap
            // ..., etc.
            std::vector<typename T::value_type> tmp;

            for (size_t ii = i - sz; ii <= i + sz; ++ii)
            {
                for (size_t jj = j - sz; jj <= j + sz; ++jj)
                {
                    if (!mask.empty () && !mask (ii, jj))
                        continue;
                    assert (ii < x.rows ());
                    assert (jj < x.cols ());
                    tmp.push_back (x (ii, jj));
                }
            }

            // The center is not masked, so we are guaranteed at least one sample
            assert (tmp.size () > 0);

            r (i, j) = median (tmp);
        }
    }

    return r;
}

/// @brief Unmasked version of median_2D_filter()
template <typename T>
T median_2D_filter (const T &x, const size_t sz)
{
    // Empty mask
    T mask;
    return median_2D_filter (x, sz, mask);
}

/// @brief Transpose an image container
/// @tparam T Container type
/// @param x Container
/// @return Transposed image
template<typename T>
T transpose (const T &x)
{
    // Allocate transposed image
    T y (x.cols (), x.rows ());

    // Transpose
#pragma omp parallel for
    for (size_t i = 0; i < x.rows (); ++i)
        for (size_t j = 0; j < x.cols (); ++j)
            y (j, i) = x (i, j);

    return y;
}

namespace detail
{

/// @brief Box 1D filter helper function
template<typename T, typename U>
typename T::value_type get_row_average (const T &sums, const U &totals, const size_t sz, const size_t len, const size_t i)
{
    // Check args
    assert (sums.size () == totals.size ());
    assert (i < sums.size ());

    // Get sums and totals on the edges of the filter window
    const int i1 = i - sz / 2 - 1;
    const int i2 = i + sz / 2;

    // Clip the index values to the edges of the row
    const double sum1 = (i1 < 0) ? 0 : sums[i1];
    const size_t total1 = (i1 < 0) ? 0 : totals[i1];
    const double sum2 = (i2 >= static_cast<int>(len)) ? sums[len - 1] : sums[i2];
    const size_t total2 = (i2 >= static_cast<int>(len)) ? totals[len - 1] : totals[i2];

    // Compute sum and total at pixel 'i'.
    //
    // The sums array contains the cumulative sums, so we can just subtract the first sum from the second sum to get the
    // sum across the part of the row that we are interested in. The same goes for the totals array.
    //
    const double sum = sum2 - sum1;
    const int total = total2 - total1;
    assert (total > 0);

    // Return the average over the window of size 'sz' at pixel 'i'
    return sum / total;
}

} // namespace detail

/// @brief Box filter a 1D array of pixels
/// @tparam T Pixel iterator type
/// @tparam U Mask iterator type
/// @param p_begin Pixel iterator
/// @param p_end Pixel iterator
/// @param sz Kernel size
/// @param mask Mask iterator
///
/// The filtering is done in-place because it is a support routine that operates on an image row.
template<typename T, typename U>
void box_1D_filter (T p_begin, const T p_end, const size_t sz, const U mask)
{
    T p = p_begin;
    const size_t len = p_end - p_begin;

    // Keep cumulative sums and totals across the array
    std::vector<double> sums (len);
    std::vector<size_t> totals (len);
    double cumulative_sum = 0.0;
    size_t cumulative_total = 0;

    // For each pixel get cumulative sums and counts
    for (size_t i = 0; i < len; ++i)
    {
        // If this pixel contains data, update sums and counts
        if (mask[i])
        {
            cumulative_sum += p[i];
            ++cumulative_total;
        }
        // Remember them
        sums[i] = cumulative_sum;
        totals[i] = cumulative_total;
    }

    // Now go back and fill in filter values based upon sums and totals
    for (size_t i = 0; i < len; ++i)
    {
        // If the pixel contains no data, skip it
        if (!mask[i])
            continue;
        // Get the box filter average
        p[i] = detail::get_row_average (sums, totals, sz, len, i);
    }
}

/// @brief Box filter a 1D array of pixels
/// @tparam T Pixel iterator type
/// @param p_begin Pixel iterator
/// @param p_end Pixel iterator
/// @param sz Kernel size
///
/// The filtering is done in-place because it is a support routine that operates on an image row.
template<typename T>
void box_1D_filter (T p_begin, const T p_end, const size_t sz)
{
    T p = p_begin;
    const size_t len = p_end - p_begin;

    // Keep cumulative sums and totals across the array
    std::vector<double> sums (len);
    std::vector<size_t> totals (len);
    double cumulative_sum = 0.0;
    size_t cumulative_total = 0;

    // For each pixel get cumulative sums and counts
    for (size_t i = 0; i < len; ++i)
    {
        // Update counts
        cumulative_sum += p[i];
        ++cumulative_total;
        // Remember them
        sums[i] = cumulative_sum;
        totals[i] = cumulative_total;
    }

    // Now go back and fill in filter values based upon sums and totals
    for (size_t i = 0; i < len; ++i)
    {
        // Get the box filter average
        p[i] = detail::get_row_average (sums, totals, sz, len, i);
    }
}

/// @brief Get a 2D box filtered image
/// @tparam T Container type
/// @tparam U Mask type
/// @param x Container
/// @param sz Kernel size
/// @param mask The mask
/// @return Filtered image
template<typename T, typename U>
T box_2D_filter (const T &x, const size_t sz, const U &mask)
{
    // Check args
    assert (mask.empty () || x.size () == mask.size ());

    // Return value
    T r (x);

    // Run a 1D filter across each row
#pragma omp parallel for
    for (size_t i = 0; i < r.rows (); ++i)
    {
        if (mask.empty ())
            box_1D_filter (&r (i, 0), &r (i + 1, 0), sz);
        else
            box_1D_filter (&r (i, 0), &r (i + 1, 0), sz, &mask (i, 0));
    }

    // Transpose
    r = transpose (r);
    const U mask2 = transpose (mask);

    // Run a 1D filter across each row again
    //
    // These rows correspond to the filtered columns of the original
#pragma omp parallel for
    for (size_t i = 0; i < r.rows (); ++i)
    {
        if (mask.empty ())
            box_1D_filter (&r (i, 0), &r (i + 1, 0), sz);
        else
            box_1D_filter (&r (i, 0), &r (i + 1, 0), sz, &mask2 (i, 0));
    }

    // Transpose back
    return transpose (r);
}

/// @brief Unmasked version of box_2D_filter
template<typename T>
T box_2D_filter (const T &x, const size_t sz)
{
    // Empty mask
    T mask;
    return box_2D_filter (x, sz, mask);
}

/// @brief Get the ideal filter width of a box filter that approximates a Gaussian filter
/// @param stddev Standard deviation of the Gaussian filter
/// @param iterations The number of iterations that the box filter is applied
/// @return Ideal box filter width
///
/// When approximating a Gaussian filter, this function calculates the box filter width that should be used.
inline double ideal_filter_width (const double stddev, const size_t iterations)
{
    return std::sqrt ((12.0 * stddev * stddev) / iterations + 1.0);
}

/// @brief Given the width of a box filter, what is the stddev of Gaussian filter that it approximates
/// @param w Box filter width
/// @param iterations The number of iterations that the box filter is applied
/// @return Stddev of the Gaussian filter
///
/// When approximating a Gaussian filter, this function calculates the box filter width that should be used.
/// inline double inverse_ideal_filter_width (const unsigned width, const size_t iterations)
/// {
///     return std::sqrt (iterations * (width * width - 1.0) / 12.0);
/// }

/// @brief Get the standard deviation of a Gaussian that is appoximated by a box filter
/// @param filter_width Width of the box filter
/// @param iterations The number of iterations that the box filter is applied
/// @return Stddev of the approximated Gaussian
inline double avg_stddev (const double filter_width, const size_t iterations)
{
    return std::sqrt ((iterations * filter_width * filter_width - iterations) / 12.0);
}

/// @brief Get a 2D gaussian filtered image
/// @tparam T Container type
/// @tparam U Mask type
/// @param x Container
/// @param stddev Standard deviation of kernel, see note below
/// @param mask The mask
/// @return Filtered image
/// @cite "Kovesi, Peter. "Fast almost-gaussian filtering." Digital Image
///        Computing: Techniques and Applications (DICTA), 2010 International Conference
///        on. IEEE, 2010."
///
/// The 'stddev' parameter is only an approximation. A square filter is applied iteratively to generate the Gaussian,
/// and its kernel size is specified by an integer. A non-integral kernel size would give a closer estimate of the
/// standard deviation of the Gaussian.
template<typename T, typename U>
T gaussian_2D_filter (const T &x, const double stddev, const U &mask)
{
    // Number of iterations used to approximate
    const size_t iterations = 3;

    // Get the ideal box filter kernel size
    const double w = ideal_filter_width (stddev, iterations);

    // The kernel size only accepts integers
    size_t sz = std::round (w);

    // Make sure the filter size is big enough.
    sz = std::max (sz, 3ul);

    // Make sure the filter size is odd.
    sz |= 0x01;

    // Approximate a Gaussian filter iteratively applying a box filter
    T r (x);
    for (size_t i = 0; i < iterations; ++i)
        r = box_2D_filter (r, sz, mask);
    return r;
}

/// @brief Unmasked version of gaussian_2D_filter
template<typename T>
T gaussian_2D_filter (const T &x, const double stddev)
{
    // An empty mask
    T mask;
    return gaussian_2D_filter (x, stddev, mask);
}

/// @brief Box grayscale dilate a 1D array of pixels
/// @tparam T Container type
/// @tparam P Dilation policy type
/// @param x Container
/// @param sz Kernel size
/// @param p Dilation policy
///
/// The filtering is done in-place because it is a support routine that operates on an image row.
template<typename T, typename P>
T box_1D_grayscale_dilation (const T &x, const size_t sz, P p)
{
    if (x.empty ())
        return T ();

    // Copy the container
    T y (x);

    // For each pixel
    for (size_t i = 0; i < x.size (); ++i)
    {
        // Get index of the start pixel, inclusive
        size_t start = i < sz ? 0 : i - sz;
        // Get index of the end pixel, non-inclusive
        size_t end = i + sz <= x.size () ? i + sz : x.size ();
        // Set the pixel
        y[i] = p (x, start, end);
    }

    // Return the result
    return y;
}

/// @brief Box grayscale dilate a 2D array of pixels
/// @tparam T Container type
/// @tparam P Dilation filter policy type
/// @param x Container
/// @param sz Kernel size
/// @param p Dilation filter policy
///
/// This is the generic version of the function, which requires a policy
/// class to define the filter policy of the square sliding window.
///
/// A version with a default maximum square sliding window is defined
/// below.
template<typename T, typename P>
T box_2D_grayscale_dilation (const T &x, const size_t sz, P p)
{
    if (x.empty ())
        return T ();

    // Copy the container
    T y (x);

    // This is a separable function, so we can operate on the rows,
    // transpose, operate on the rows again, and then transpose again.
    //
    // For each row
    for (size_t i = 0; i < x.rows (); ++i)
    {
        // For each column
        for (size_t j = 0; j < x.cols (); ++j)
        {
            // Get index of the start pixel, inclusive
            const size_t start_col = (j <= sz) ? 0 : j - sz;
            // Get index of the end pixel, non-inclusive
            const size_t end_col = (j + sz + 1 <= x.cols ()) ? j + sz + 1 : x.cols ();
            // Set the pixel
            y (i, j) = p (x, i, start_col, end_col);
        }
    }

    // Transpose so we can do the same operation on the transposed rows
    y = transpose (y);

    // Copy the container
    T z (y);

    // For each transposed row
    for (size_t i = 0; i < y.rows (); ++i)
    {
        // For each transposed column
        for (size_t j = 0; j < y.cols (); ++j)
        {
            // Get index of the start pixel, inclusive
            size_t start_col = j < sz ? 0 : j - sz;
            // Get index of the end pixel, non-inclusive
            size_t end_col = j + sz + 1 <= y.cols () ? j + sz + 1 : y.cols ();
            // Set the pixel
            z (i, j) = p (y, i, start_col, end_col);
        }
    }

    // Transpose back to original shape
    return transpose (z);
}

/// @brief Helper function for dilation
/// @tparam T Container type
/// @param x Container
/// @param i Container row
/// @param start_col Container start column
/// @param end_col Container end column
template<typename T>
typename T::value_type max_filter (const T &x, const size_t i, const size_t start_col, const size_t end_col)
{
    assert (i < x.rows ());
    assert (start_col < x.cols ());
    assert (end_col <= x.cols ());

    auto max = x (i, start_col);
    for (size_t j = start_col + 1; j < end_col; ++j)
        max = std::max (max, x (i, j));
    return max;
}

/// @brief Box grayscale dilate a 2D array of pixels
/// @tparam T Container type
/// @param x Container
/// @param sz Kernel size
///
/// This version uses a maximum filter over a square sliding window.
template<typename T>
T box_2D_grayscale_dilation (const T &x, const size_t sz)
{
    return box_2D_grayscale_dilation (x, sz, max_filter<T>);
}

/// @brief Return a list of indices above a given peak value
/// @tparam T Container type
/// @param x Container
/// @return Peak container
template <typename T>
std::vector<size_t> find_peaks(const T &x)
{
    std::vector<size_t> peaks;

    if (x.size() < 3)
        return peaks;

    // If the center value is greater than value on the left and right...
    for (size_t index = 1; index + 1 < x.size(); ++index)
        if (x[index-1] < x[index] && x[index+1] < x[index])
            peaks.push_back(index);

    return peaks;
}

/// @brief Rotate a raster 90 degrees clockwise
/// @tparam T Container type
/// @param x Container
/// @return Rotated image
template<typename T>
T rotate90_cw (const T &x)
{
    // Allocate rotated image
    T y (x.cols (), x.rows ());

    // Rotate
#pragma omp parallel for
    for (size_t i = 0; i < y.rows (); ++i)
        for (size_t j = 0; j < y.cols (); ++j)
            y (i, j) = x (x.rows () - j - 1, i);

    return y;
}

/// @brief Rotate a raster 90 degrees counter-clockwise
/// @tparam T Container type
/// @param x Container
/// @return Rotated image
template<typename T>
T rotate90_ccw (const T &x)
{
    // Allocate rotated image
    T y (x.cols (), x.rows ());

    // Rotate
#pragma omp parallel for
    for (size_t i = 0; i < y.rows (); ++i)
        for (size_t j = 0; j < y.cols (); ++j)
            y (i, j) = x (j, x.cols () - i - 1);

    return y;
}

/// @brief Rotate a raster 180 degrees
/// @tparam T Container type
/// @param x Container
/// @return Rotated image
template<typename T>
T rotate180 (const T &x)
{
    return rotate90_cw (rotate90_cw (x));
}

/// @brief Check if a container contains a nan
/// @tparam T Container type
/// @param x Container
/// @return true if there are any nans, false otherwise
template<typename T>
bool contains_nan(const T &x)
{
    for (size_t i = 0; i < x.size(); i++)
    {
        if (std::isnan(x[i]))
            return true;
    }
    return false;
}

/// @brief Randomly order a container
///
/// Use a unique seed for each call to get a difference random order
///
/// Be deterministic -- don't do something foolish like set the seed to the timestamp
template<typename T>
T random_shuffle (const T &x, const size_t seed)
{
    auto y (x);

    // Init RNG
    std::minstd_rand rng (seed);

    // Randomize
    for (size_t i = 0; i < x.size (); ++i)
        std::swap (y[i], y[rng () % y.size ()]);

    return y;
}

} // namespace container_utils

} // namespace ATL24_coastnet

#endif // CONTAINER_UTILS_H
