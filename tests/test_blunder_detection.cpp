#include "ATL24_coastnet/coastnet.h"
#include "ATL24_coastnet/verify.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;
using namespace ATL24_coastnet;

void test_along_track_distance ()
{
    vector<classified_point2d> p;
    //  h5_index, x, z, cls
    p.push_back ({0, 1, 0, 41});

    VERIFY (false);
}

void test_surface_estimate ()
{
    VERIFY (false);
}

void test_bathy_estimate ()
{
    VERIFY (false);
}

void test_empty ()
{
    VERIFY (false);
}

void test_gauss ()
{
    VERIFY (false);
}

void test_no_surface ()
{
    VERIFY (false);
}

void test_no_bathy ()
{
    VERIFY (false);
}

void test_surface ()
{
    VERIFY (false);
}

void test_bathy ()
{
    VERIFY (false);
}

int main ()
{
    try
    {
        test_along_track_distance ();
        test_surface_estimate ();
        test_bathy_estimate ();
        test_empty ();
        test_gauss ();
        test_no_surface ();
        test_no_bathy ();
        test_surface ();
        test_bathy ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
