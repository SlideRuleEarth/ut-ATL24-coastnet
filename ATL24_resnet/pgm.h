#pragma once

#include "viper/raster.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace ATL24_resnet
{

namespace pgm
{

struct header
{
    header () : w (0), h (0)
    { }
    header (const size_t w, const size_t h)
        : w (w)
        , h (h)
    { }
    size_t w; // Width of image in pixels
    size_t h; // Height of images in pixels
};

void write_header (
    std::ostream &os,
    const header &h,
    const std::string &comment = std::string ())
{
    // Write the magic number
    os << "P5" << "\n";

    // Write a comment if one was given
    if (!comment.empty ())
        os << "# " << comment << std::endl;

    // Write the dimensions
    os << h.w << ' ' << h.h << '\n';
    // 8 bits per pixel
    os << "255\n";
}

void read_comment (std::istream &s)
{
    // Ignore whitespace
    s >> std::ws;
    // A '#' marks a comment line
    while (s.peek () == '#')
    {
        const unsigned LINE_LENGTH = 256;
        char line[LINE_LENGTH];
        s.getline (line, LINE_LENGTH);
        // Ignore whitespace
        s >> std::ws;
    }
}

header read_header (std::istream &is)
{
    char ch;
    is >> ch;
    if (ch != 'P')
        throw std::runtime_error ("Invalid PNM magic number");
    is >> ch;
    header h;
    switch (ch)
    {
        case '5':
        break;
        case '6':
        throw std::runtime_error ("RGB is not supported");
        break;
        default:
        throw std::runtime_error ("Unknown PNM magic number");
    }
    read_comment (is);
    is >> h.w;
    read_comment (is);
    is >> h.h;
    unsigned maxval;
    is >> maxval;
    if (maxval >= 256)
        throw std::runtime_error ("16-bit pixels is not supported");
    // Read a single WS
    is.get (ch);
    return h;
}

void write (std::ostream &os,
    const header &h,
    const viper::raster::raster<unsigned char> &r,
    const std::string &comment = std::string ())
{
    if (r.rows () != h.h)
        throw std::runtime_error ("pgm::write(): The header does not match the raster height");

    if (r.cols () != h.w)
        throw std::runtime_error ("pgm::write(): The header does not match the raster width");

    write_header (os, h, comment);

    // Write in native endianess. This is not portable.
    os.write (reinterpret_cast<const char *> (&r[0]), h.h * h.w);
}

header read (std::istream &is, viper::raster::raster<unsigned char> &r)
{
    const auto h = read_header (is);

    // Allocate space for pixels
    r = viper::raster::raster<unsigned char> (h.h, h.w);

    // Read the pixels
    is.read (reinterpret_cast<char *> (&r[0]), r.size ());

    return h;
}

} // namespace pgm

} // namespace ATL24_resnet
