#include "ATL24_coastnet/pgm.h"
#include "ATL24_coastnet/verify.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;
using namespace viper::raster;
using namespace ATL24_coastnet::pgm;

void test_bad_headers ()
{
    stringstream s;
    header h;
    {
        // Wrong the magic number
        s.str ("X5\n1 1\n255\n");
        bool failed = false;
        try { h = read_header (s); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }
    {
        // Wrong the file spec number
        s.str ("P9\n1 1\n255\n");
        bool failed = false;
        try { h = read_header (s); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }
    {
        // Wrong maxsize
        stringstream s;
        s.str ("P5\n1 1\n255555\n");
        bool failed = false;
        try { h = read_header (s); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }
}

void test_read_write_gs8 (const string &comment = string ())
{
    const size_t W = 13;
    const size_t H = 17;
    raster<unsigned char> p1 (H, W, 'A');
    header h1 (W, H);
    stringstream s;

    // Write it
    write (s, h1, p1, comment);

    // Read it back
    raster<unsigned char> p2;
    header h2 = read (s, p2);
    VERIFY (h1.w == h2.w);
    VERIFY (h1.h == h2.h);
    VERIFY (p1 == p2);
}

void test_failed_read_write ()
{
    {
        // Wrong buffer size
        stringstream s;
        header h (1, 2);
        bool failed = false;
        try { raster<unsigned char> p (1, 1); write (s, h, p); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }

    {
        // Wrong buffer size
        stringstream s;
        header h (2, 1);
        bool failed = false;
        try { raster<unsigned char> p (1, 1); write (s, h, p); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }

    {
        // Wrong pixel size
        stringstream s;
        s.str ("P5\n1 1\n65535\n");
        header h;
        bool failed = false;
        try { raster<unsigned char> p (1, 1); h = read (s, p); }
        catch (...) { failed = true; }
        VERIFY (failed);
    }
}

int main ()
{
    try
    {
        test_bad_headers ();
        test_read_write_gs8 ();
        test_read_write_gs8 ("comment");
        test_failed_read_write ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
