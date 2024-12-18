#include "blunder_detection.h"
#include "coastnet.h"
#include "verify.h"

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
    const auto x1 = get_surface_estimates (p, 2.0);
    VERIFY (x1.empty ());
    const auto x2 = get_bathy_estimates (p, 2.0);
    VERIFY (x2.empty ());
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

void test_get_quantized_variance ()
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

    auto a0 = detail::get_quantized_variance (p, 0, 1.0);
    auto a1 = detail::get_quantized_variance (p, 1, 1.0);
    auto a2 = detail::get_quantized_variance (p, 2, 1.0);

    VERIFY (a0.size () == p.size ());
    VERIFY (about_equal (a0[0], 0));
    VERIFY (about_equal (a0[1], 0));
    VERIFY (a0[2] > 0.0);
    VERIFY (a0[3] > 0.0);
    VERIFY (a0[4] > 0.0);
    VERIFY (a0[5] > 0.0);
    VERIFY (a0[6] > 0.0);
    VERIFY (about_equal (a0[7], 0));

    VERIFY (a1.size () == p.size ());
    VERIFY (std::isnan (a1[0]));
    VERIFY (std::isnan (a1[1]));
    VERIFY (!std::isnan (a1[2]));
    VERIFY (!std::isnan (a1[3]));
    VERIFY (!std::isnan (a1[4]));
    VERIFY (!std::isnan (a1[5]));
    VERIFY (!std::isnan (a1[6]));
    VERIFY (std::isnan (a1[7]));

    VERIFY (a2.size () == p.size ());
    VERIFY (std::isnan (a2[0]));
    VERIFY (std::isnan (a2[1]));
    VERIFY (std::isnan (a2[2]));
    VERIFY (std::isnan (a2[3]));
    VERIFY (std::isnan (a2[4]));
    VERIFY (std::isnan (a2[5]));
    VERIFY (std::isnan (a2[6]));
    VERIFY (std::isnan (a2[7]));
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

    const auto x = detail::get_quantized_variance (p, sea_surface_class, 1.0);
    for (auto i : x)
        VERIFY (std::isnan (i));
}

void test_no_bathy ()
{
    vector<classified_point2d> p;

    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, 0, 41});

    auto z = get_surface_estimates (p, 2.0);
    VERIFY (!z.empty ());

    const auto x = detail::get_quantized_variance (p, bathy_class, 1.0);
    for (auto i : x)
        VERIFY (std::isnan (i));
}

void test_bathy_depth_check ()
{
    {
    vector<classified_point2d> p;

    // Surface only
    for (size_t i = 0; i < 100; ++i)
        p.push_back ({0, 1, 0, 0, 41});

    // No bathy, so no change
    auto q = detail::bathy_depth_check (p, 1.0, 2.0);
    VERIFY (p == q);
    }

    {
    vector<classified_point2d> p;

    // Bathy only
    for (size_t i = 0; i < 100; ++i)
        p.push_back ({0, 1, 0, 0, 40});

    // No surface, so no bathy...
    auto q = detail::bathy_depth_check (p, 1.0, 2.0);
    for (size_t i = 0; i < 100; ++i)
        VERIFY (q[i].prediction != 40);
    }

    {
    vector<classified_point2d> p;

    // h5_index, x, z, cls, prediction, surface_elevation, bathy_elevation
    // Add surface at 0.0
    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, 0.0, 0, 41});

    // Add bathy at 1.0
    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, 1.0, 0, 40});

    // Add bathy at -1.0
    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, -1.0, 0, 40});

    auto q = detail::bathy_depth_check (p, 1.0, 2.0);

    // These got changed
    for (size_t i = 10; i < 20; ++i)
        VERIFY (q[i].prediction == 0);

    // These did not change
    for (size_t i = 20; i < 30; ++i)
        VERIFY (q[i].prediction == 40);
    }

    {
    vector<classified_point2d> p;

    // h5_index, x, z, cls, prediction, surface_elevation, bathy_elevation
    //
    // Add surface at -3.0
    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, -3.0, 0, 41});

    // More surface
    for (size_t i = 0; i < 10; ++i)
        p.push_back ({0, 1, -3.5, 0, 41});

    const auto var = detail::get_quantized_variance (p, sea_surface_class, 1.0);

    // Error is 0.25 at each photon, so var=0.25^2=0.0625
    for (size_t i = 0; i < var.size (); ++i)
        VERIFY (about_equal (var[i], 0.0625));

    // Add bathy
    p.push_back ({0, 1, 10.00, 0, 40}); // No
    p.push_back ({0, 1,  0.00, 0, 40}); // No
    p.push_back ({0, 1, -3.00, 0, 40}); // No
    p.push_back ({0, 1, -3.74, 0, 40}); // No
    p.push_back ({0, 1, -3.76, 0, 40}); // Yes
    p.push_back ({0, 1, -5.00, 0, 40}); // Yes

    // Get surface estimates
    postprocess_params params;
    auto z = get_surface_estimates (p, params.surface_sigma);

    // Assign them
    VERIFY (z.size () == p.size ());
    for (size_t i = 0; i < p.size (); ++i)
        p[i].surface_elevation = z[i];

    auto q = detail::bathy_depth_check (p, 1.0, 2.0);

    // These got changed
    VERIFY (q[20].prediction == 0);
    VERIFY (q[21].prediction == 0);
    VERIFY (q[22].prediction == 0);
    VERIFY (q[23].prediction == 0);

    // These did not
    VERIFY (q[24].prediction == 40);
    VERIFY (q[25].prediction == 40);
    }
}

void test_filter_isolated_bathy ()
{
    {
    vector<classified_point2d> p;

    // h5_index, x, z, cls, prediction, surface_elevation, bathy_elevation
    p.push_back ({0, 0.0, 0.0, 0, 40});
    p.push_back ({1, 0.0, 0.0, 0, 41});
    p.push_back ({2, 0.0, 6.0, 0, 40});
    p.push_back ({3, 5.0, 0.0, 0, 40});
    p.push_back ({4, 6.0, 0.0, 0, 40});
    p.push_back ({5, 7.0, 5.0, 0, 40});

    const double r = 4.0;
    const double n = 2;
    p = detail::filter_isolated_bathy (p, r, n);

    VERIFY (p[0].prediction == 0); // reclass
    VERIFY (p[1].prediction == 41);
    VERIFY (p[2].prediction == 0); // reclass
    VERIFY (p[3].prediction == 40);
    VERIFY (p[4].prediction == 40);
    VERIFY (p[5].prediction == 0); // reclass
    }

    {
    vector<classified_point2d> p;

    // h5_index, x, z, cls, prediction, surface_elevation, bathy_elevation
    p.push_back ({0, 0.0, 0.0, 0, 40});
    p.push_back ({1, 1.0, -1.0, 0, 40});
    p.push_back ({2, 1.0, 0.0, 0, 40});
    p.push_back ({3, 1.0, 1.0, 0, 40});
    p.push_back ({4, 2.0, 0.0, 0, 40});
    p.push_back ({5, 3.0, -1.0, 0, 40});
    p.push_back ({6, 3.0, 0.0, 0, 40});
    p.push_back ({7, 3.0, 1.0, 0, 40});
    p.push_back ({8, 4.0, 0.0, 0, 40});

    auto q = detail::filter_isolated_bathy (p, 100, 3);

    for (size_t i = 0; i < q.size (); ++i)
        VERIFY (q[i].prediction == 40); // no reclass

    q = detail::filter_isolated_bathy (p, 0.1, 3);

    for (size_t i = 0; i < q.size (); ++i)
        VERIFY (q[i].prediction == 0); // reclass

    q = detail::filter_isolated_bathy (p, 1.1, 5);

    for (size_t i = 0; i < q.size (); ++i)
        VERIFY (q[i].prediction == 40); // no reclass
    }

    {
    vector<classified_point2d> p;

    // h5_index, x, z, cls, prediction, surface_elevation, bathy_elevation
    p.push_back ({0, 0.0, 0.0, 0, 40});
    p.push_back ({1, 1.0, -1.0, 0, 40});
    p.push_back ({2, 1.0, 0.0, 0, 40});
    p.push_back ({3, 1.0, 1.0, 0, 40});
    p.push_back ({4, 2.0, 0.0, 0, 40});
    p.push_back ({5, 4.0, 0.0, 0, 40});
    p.push_back ({6, 6.0, 0.0, 0, 40});
    p.push_back ({7, 7.0, -1.0, 0, 40});
    p.push_back ({8, 7.0, 0.0, 0, 40});
    p.push_back ({9, 7.0, 1.0, 0, 40});

    auto q = detail::filter_isolated_bathy (p, 1.1, 5);

    VERIFY (q[0].prediction == 40);
    VERIFY (q[1].prediction == 40);
    VERIFY (q[2].prediction == 40);
    VERIFY (q[3].prediction == 40);
    VERIFY (q[4].prediction == 40);
    VERIFY (q[5].prediction == 0); // reclass
    VERIFY (q[6].prediction == 0); // reclass
    VERIFY (q[7].prediction == 0); // reclass
    VERIFY (q[8].prediction == 0); // reclass
    VERIFY (q[9].prediction == 0); // reclass

    p.push_back ({10, 8.0, 0.0, 0, 40});
    q = detail::filter_isolated_bathy (p, 1.1, 5);

    VERIFY (q[0].prediction == 40);
    VERIFY (q[1].prediction == 40);
    VERIFY (q[2].prediction == 40);
    VERIFY (q[3].prediction == 40);
    VERIFY (q[4].prediction == 40);
    VERIFY (q[5].prediction == 0); // reclass
    VERIFY (q[6].prediction == 40);
    VERIFY (q[7].prediction == 40);
    VERIFY (q[8].prediction == 40);
    VERIFY (q[9].prediction == 40);

    q = detail::filter_isolated_bathy (p, 0.9, 5);

    for (size_t i = 0; i < q.size (); ++i)
        VERIFY (q[i].prediction == 0); // no reclass
    }
}

int main ()
{
    try
    {
        test_empty ();
        test_count_photons ();
        test_get_quantized_average ();
        test_get_quantized_variance ();
        test_get_nan_pairs ();
        test_interpolate_nans ();
        test_box_filter ();
        test_get_surface_estimates ();
        test_get_bathy_estimates ();
        test_no_surface ();
        test_no_bathy ();
        test_bathy_depth_check ();
        test_filter_isolated_bathy ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
