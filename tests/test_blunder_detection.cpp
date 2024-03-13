#include "ATL24_coastnet/coastnet.h"
#include "ATL24_coastnet/verify.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;
using namespace ATL24_coastnet;

bool about_equal (double a, double b, unsigned precision = 3)
{
    int c = static_cast<int> (std::round (a * std::pow (10.0, precision)));
    int d = static_cast<int> (std::round (b * std::pow (10.0, precision)));
    return c == d;
}

void test_empty ()
{
    vector<classified_point2d> p;
    const auto x1 = get_nearest_along_track_prediction (p, 0);
    VERIFY (x1.empty ());
    const auto x2 = get_surface_estimates (p, 2.0);
    VERIFY (x2.empty ());
    const auto x3 = get_bathy_estimates (p, 2.0);
    VERIFY (x3.empty ());
}

void test_get_nearest_along_track_photon ()
{
    vector<classified_point2d> p;

    //  h5_index, x, z, cls, pred
    p.push_back ({0, 1, 0, 0, 41});
    p.push_back ({1, 2, 100, 0, 0});
    p.push_back ({2, 3, 100, 0, 0});
    p.push_back ({3, 10, 200, 0, 40});
    p.push_back ({4, 11, 300, 0, 0});

    const auto indexes_0 = get_nearest_along_track_prediction (p, 0);
    const auto indexes_40 = get_nearest_along_track_prediction (p, 40);
    const auto indexes_41 = get_nearest_along_track_prediction (p, 41);
    const auto indexes_123 = get_nearest_along_track_prediction (p, 123);

    VERIFY (indexes_0[0] == 1);
    VERIFY (indexes_0[1] == 1);
    VERIFY (indexes_0[2] == 2);
    VERIFY (indexes_0[3] == 4);
    VERIFY (indexes_0[4] == 4);

    VERIFY (indexes_40[0] == 3);
    VERIFY (indexes_40[1] == 3);
    VERIFY (indexes_40[2] == 3);
    VERIFY (indexes_40[3] == 3);
    VERIFY (indexes_40[4] == 3);

    VERIFY (indexes_41[0] == 0);
    VERIFY (indexes_41[1] == 0);
    VERIFY (indexes_41[2] == 0);
    VERIFY (indexes_41[3] == 0);
    VERIFY (indexes_41[4] == 0);

    for (auto i : indexes_123)
        VERIFY (i == indexes_123.size ());
}

void test_count_photons ()
{
    vector<classified_point2d> p;

    //  h5_index, x, z, cls, pred
    p.push_back ({0, 1, 0, 0, 41});
    p.push_back ({1, 2, 100, 0, 0});
    p.push_back ({2, 3, 100, 0, 0});
    p.push_back ({3, 10, 200, 0, 40});
    p.push_back ({4, 11, 300, 0, 0});

    VERIFY (count_predictions (p, 0) == 3);
    VERIFY (count_predictions (p, 40) == 1);
    VERIFY (count_predictions (p, 41) == 1);
}

void test_get_quantized_average ()
{
    vector<classified_point2d> p;

    //  h5_index, x, z, cls, pred
    p.push_back ({0, 1.1, 100, 0, 0});
    p.push_back ({1, 2.0, 101, 0, 0});
    p.push_back ({2, 3.1, 102, 0, 0});
    p.push_back ({3, 3.2, 103, 0, 1});
    p.push_back ({4, 3.5, 104, 0, 0});
    p.push_back ({5, 3.6, 105, 0, 0});
    p.push_back ({6, 3.7, 106, 0, 0});
    p.push_back ({7, 4.6, 107, 0, 0});

    auto a0 = get_quantized_average (p, 0);
    auto a1 = get_quantized_average (p, 1);
    auto a2 = get_quantized_average (p, 2);

    VERIFY (a0.size () == 4);
    VERIFY (about_equal (a0[0], 100));
    VERIFY (about_equal (a0[1], 101));
    VERIFY (about_equal (a0[2], 104.25));
    VERIFY (about_equal (a0[3], 107));

    VERIFY (a1.size () == 4);
    VERIFY (std::isnan (a1[0]));
    VERIFY (std::isnan (a1[1]));
    VERIFY (about_equal (a1[2], 103));
    VERIFY (std::isnan (a1[3]));

    VERIFY (a2.size () == 4);
    VERIFY (std::isnan (a2[0]));
    VERIFY (std::isnan (a2[1]));
    VERIFY (std::isnan (a2[2]));
    VERIFY (std::isnan (a2[3]));
}

void test_get_nan_pairs ()
{
    vector<double> p0;
    p0.push_back (NAN);
    p0.push_back (0.0);
    p0.push_back (NAN);
    const auto np0 = get_nan_pairs (p0);
    VERIFY (np0.size () == 2);
    VERIFY (np0[0].first == 0);
    VERIFY (np0[0].second == 1);
    VERIFY (np0[1].first == 1);
    VERIFY (np0[1].second == 2);

    vector<double> p1;
    p1.push_back (0.0);
    p1.push_back (NAN);
    p1.push_back (NAN);
    p1.push_back (0.0);
    const auto np1 = get_nan_pairs (p1);
    VERIFY (np1.size () == 1);
    VERIFY (np1[0].first == 0);
    VERIFY (np1[0].second == 3);

    vector<double> p2;
    p2.push_back (0.0);
    p2.push_back (NAN);
    p2.push_back (NAN);
    p2.push_back (NAN);
    p2.push_back (0.0);
    p2.push_back (0.0);
    p2.push_back (NAN);
    p2.push_back (0.0);
    const auto np2 = get_nan_pairs (p2);
    VERIFY (np2.size () == 2);
    VERIFY (np2[0].first == 0);
    VERIFY (np2[0].second == 4);
    VERIFY (np2[1].first == 5);
    VERIFY (np2[1].second == 7);

    vector<double> p3;
    p3.push_back (0.0);
    p3.push_back (0.0);
    p3.push_back (NAN);
    p3.push_back (NAN);
    p3.push_back (NAN);
    p3.push_back (0.0);
    p3.push_back (NAN);
    const auto np3 = get_nan_pairs (p3);

    VERIFY (np3.size () == 2);
    VERIFY (np3[0].first == 1);
    VERIFY (np3[0].second == 5);
    VERIFY (np3[1].first == 5);
    VERIFY (np3[1].second == 6);
}

void test_interpolate_nans ()
{
    vector<double> p0;
    p0.push_back (NAN);
    p0.push_back (1.0);
    p0.push_back (NAN);
    const auto np0 = get_nan_pairs (p0);
    for (auto n : np0)
        interpolate_nans (p0, n);
    VERIFY (about_equal (p0[0], 1.0));
    VERIFY (about_equal (p0[1], 1.0));
    VERIFY (about_equal (p0[2], 1.0));

    vector<double> p1;
    p1.push_back (1.0);
    p1.push_back (NAN);
    p1.push_back (NAN);
    p1.push_back (4.0);
    const auto np1 = get_nan_pairs (p1);
    for (auto n : np1)
        interpolate_nans (p1, n);
    VERIFY (about_equal (p1[0], 1.0));
    VERIFY (about_equal (p1[1], 2.0));
    VERIFY (about_equal (p1[2], 3.0));
    VERIFY (about_equal (p1[3], 4.0));

    vector<double> p2;
    p2.push_back (1.0);
    p2.push_back (NAN);
    p2.push_back (NAN);
    p2.push_back (NAN);
    const auto np2 = get_nan_pairs (p2);
    for (auto n : np2)
        interpolate_nans (p2, n);
    VERIFY (about_equal (p2[0], 1.0));
    VERIFY (about_equal (p2[1], 1.0));
    VERIFY (about_equal (p2[2], 1.0));
    VERIFY (about_equal (p2[3], 1.0));

    vector<double> p3;
    p3.push_back (NAN);
    p3.push_back (NAN);
    p3.push_back (NAN);
    p3.push_back (3.0);
    const auto np3 = get_nan_pairs (p3);
    for (auto n : np3)
        interpolate_nans (p3, n);
    VERIFY (about_equal (p3[0], 3.0));
    VERIFY (about_equal (p3[1], 3.0));
    VERIFY (about_equal (p3[2], 3.0));
    VERIFY (about_equal (p3[3], 3.0));
}

void test_box_filter ()
{
    vector<double> p;
    p.push_back (1.0); // 0
    p.push_back (NAN); // 1
    p.push_back (NAN); // 2
    p.push_back (4.0); // 3
    p.push_back (NAN); // 4
    p.push_back (4.0); // 5
    p.push_back (NAN); // 6
    p.push_back (NAN); // 7
    p.push_back (7.0); // 8
    p.push_back (NAN); // 9
    p.push_back (NAN); // 10
    const auto np = get_nan_pairs (p);
    for (auto n : np)
        interpolate_nans (p, n);
    VERIFY (about_equal (p[0], 1.0));
    VERIFY (about_equal (p[1], 2.0));
    VERIFY (about_equal (p[2], 3.0));
    VERIFY (about_equal (p[3], 4.0));
    VERIFY (about_equal (p[4], 4.0));
    VERIFY (about_equal (p[5], 4.0));
    VERIFY (about_equal (p[6], 5.0));
    VERIFY (about_equal (p[7], 6.0));
    VERIFY (about_equal (p[8], 7.0));
    VERIFY (about_equal (p[9], 7.0));
    VERIFY (about_equal (p[10], 7.0));
    p = box_filter (p, 3);
    VERIFY (about_equal (p[0], 1.5));
    VERIFY (about_equal (p[1], 2.0));
    VERIFY (about_equal (p[2], 3.0));
    VERIFY (about_equal (p[3], 3.66667));
    VERIFY (about_equal (p[4], 4.0));
    VERIFY (about_equal (p[5], 4.33333));
    VERIFY (about_equal (p[6], 5.0));
    VERIFY (about_equal (p[7], 6.0));
    VERIFY (about_equal (p[8], 6.66667));
    VERIFY (about_equal (p[9], 7.0));
    VERIFY (about_equal (p[10], 7.0));
    p = box_filter (p, 13);
    VERIFY (!about_equal (p[0], 1.5));
    VERIFY (!about_equal (p[1], 2.0));
    VERIFY (!about_equal (p[2], 3.0));
    VERIFY (!about_equal (p[3], 3.66667));
    VERIFY (!about_equal (p[4], 4.0));
    VERIFY (!about_equal (p[5], 4.33333));
    VERIFY (!about_equal (p[6], 5.0));
    VERIFY (!about_equal (p[7], 6.0));
    VERIFY (!about_equal (p[8], 6.66667));
    VERIFY (!about_equal (p[9], 7.0));
    VERIFY (!about_equal (p[10], 7.0));
}

void test_get_surface_estimates ()
{
    vector<classified_point2d> p;

    //  h5_index, x, z, cls, pred
    p.push_back ({0, 1, 0, 0, 41});
    p.push_back ({1, 2, 100, 0, 0});
    p.push_back ({2, 3, 100, 0, 41});
    p.push_back ({3, 4, 200, 0, 41});
    p.push_back ({4, 5, 300, 0, 0});

    auto z = get_surface_estimates (p, 2.0);
    VERIFY (!z.empty ());
    VERIFY (z[0] > 0.0 && z[0] < 100);
    VERIFY (z[1] > 0.0 && z[1] < 100);
    VERIFY (z[2] > 100.0 && z[2] < 200);
    VERIFY (z[3] > 100.0 && z[3] < 200);
    VERIFY (z[4] > 100.0 && z[4] < 200);
}

void test_get_bathy_estimates ()
{
    vector<classified_point2d> p;

    //  h5_index, x, z, cls, pred
    p.push_back ({0, 1, 0, 0, 40});
    p.push_back ({1, 2, 100, 0, 0});
    p.push_back ({2, 3, 100, 0, 40});
    p.push_back ({3, 4, 200, 0, 40});
    p.push_back ({4, 5, 300, 0, 0});

    auto z = get_bathy_estimates (p, 2.0);
    VERIFY (!z.empty ());
    VERIFY (z[0] > 0.0 && z[0] < 100);
    VERIFY (z[1] > 0.0 && z[1] < 100);
    VERIFY (z[2] > 100.0 && z[2] < 200);
    VERIFY (z[3] > 100.0 && z[3] < 200);
    VERIFY (z[4] > 100.0 && z[4] < 200);
}

void test_no_surface ()
{
    vector<classified_point2d> p;

    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, 0, 40});

    auto z = get_surface_estimates (p, 2.0);
    VERIFY (!z.empty ());

    const auto x = get_nearest_along_track_prediction (p, sea_surface_class);
    for (auto i : x)
        VERIFY (i == p.size ());
}

void test_no_bathy ()
{
    vector<classified_point2d> p;

    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, 0, 41});

    auto z = get_surface_estimates (p, 2.0);
    VERIFY (!z.empty ());

    const auto x = get_nearest_along_track_prediction (p, bathy_class);
    for (auto i : x)
        VERIFY (i == p.size ());
}


int main ()
{
    try
    {
        test_empty ();
        test_get_nearest_along_track_photon ();
        test_count_photons ();
        test_get_quantized_average ();
        test_get_nan_pairs ();
        test_interpolate_nans ();
        test_box_filter ();
        test_get_surface_estimates ();
        test_get_bathy_estimates ();
        test_no_surface ();
        test_no_bathy ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
