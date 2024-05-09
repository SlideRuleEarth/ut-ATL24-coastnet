#pragma once

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace ATL24_coastnet
{

namespace dataframe
{

struct dataframe
{
    std::vector<std::string> headers;
    std::vector<std::vector<double>> columns;
    bool is_valid () const
    {
        // Number of headers match number of columns
        if (headers.size () != columns.size ())
            return false;
        // Number of rows are the same in each column
        for (size_t i = 1; i < columns.size (); ++i)
            if (columns[i].size () != columns[0].size ())
                return false;
        return true;
    }
    size_t rows () const
    {
        assert (is_valid ());
        return columns.empty () ? 0 : columns[0].size ();
    }
};

dataframe read (std::istream &is)
{
    using namespace std;

    // Create the dataframe
    dataframe df;

    // Read the headers
    string line;

    if (!getline (is, line))
        return df;

    // Parse each individual column header
    stringstream ss (line);
    string header;
    while (getline (ss, header, ','))
    {
        // Remove LFs in case the file was created under Windows
        std::erase (header, '\r');

        // Save it
        df.headers.push_back (header);
    }

    // Allocate column vectors
    df.columns.resize (df.headers.size ());

    // Now get the rows
    while (getline (is, line))
    {
        // Skip empty lines
        if (line.empty ())
            continue;
        char *p = &line[0];
        for (size_t i = 0; i < df.headers.size (); ++i)
        {
            char *end;
            const double x = strtod (p, &end);
            df.columns[i].push_back (x);
            p = end;
            // Ignore ','
            if (*p == ',')
                ++p;
        }
    }

    assert (df.is_valid ());
    return df;
}

dataframe read (const std::string &fn)
{
    using namespace std;

    ifstream ifs (fn);
    if (!ifs)
        throw runtime_error ("Could not open file for reading");
    return ATL24_coastnet::dataframe::read (ifs);
}

std::ostream &write (std::ostream &os, const dataframe &df, const size_t precision = 16)
{
    using namespace std;

    assert (df.is_valid ());

    const size_t ncols = df.headers.size ();

    // Short-circuit
    if (ncols == 0)
        return os;

    // Print headers
    bool first = true;
    for (auto h : df.headers)
    {
        if (!first)
            os << ",";
        first = false;
        os << h;
    }
    os << endl;

    const size_t nrows = df.columns[0].size ();

    // Short-circuit
    if (nrows == 0)
        return os;

    // Save format
    const auto f = os.flags();
    const auto p = os.precision();

    // Set format
    os << std::fixed;
    os << std::setprecision (precision);

    // Write it out
    for (size_t i = 0; i < nrows; ++i)
    {
        for (size_t j = 0; j < ncols; ++j)
        {
            if (j != 0)
                os << ",";
            os << df.columns[j][i];
        }
        os << endl;
    }

    // Restore format
    os.precision (p);
    os.flags (f);

    return os;
}

std::ostream &operator<< (std::ostream &os, const dataframe &df)
{
    return write (os , df);
}

}

} // namespace ATL24_coastnet
