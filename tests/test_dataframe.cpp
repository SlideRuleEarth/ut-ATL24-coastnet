#include "ATL24_coastnet/dataframe.h"
#include "ATL24_coastnet/verify.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

using namespace std;
using namespace ATL24_coastnet::dataframe;
using namespace std::chrono;

class timer
{
private:
    time_point<system_clock> t1;
    time_point<system_clock> t2;
    bool running;

public:
    timer () : running (false)
    {
        start ();
    }
    void start ()
    {
        t1 = system_clock::now ();
        running = true;
    }
    void stop ()
    {
        t2 = system_clock::now ();
        running = false;
    }
    double elapsed_ms()
    {
        return running
            ? duration_cast<milliseconds> (system_clock::now () - t1).count ()
            : duration_cast<milliseconds> (t2 - t1).count ();
    }
};

struct temp_file
{
    std::string name;
    std::random_device rng;
    temp_file ()
    {
        name = string (filesystem::temp_directory_path ())
            + string ("/")
            + to_string (rng ());
    }
    ~temp_file ()
    {
        std::filesystem::remove (name);
    }
};

void test_dataframe (const size_t cols, const size_t rows, const size_t precision)
{
    // Create the dataframe column names
    vector<string> names (cols);
    size_t n = 0;
    generate (names.begin(), names.end(), [&](){return to_string(n++);});

    // Create a dataframe
    dataframe df;
    for (const auto &i : names)
        df.add_column(i);
    df.set_rows (rows);

    // Set the data to random numbers
    mt19937 rng(12345);
    std::uniform_real_distribution<> d(1.0, 100.0);
    for (const auto &name : df.get_headers ())
        for (size_t i = 0; i < rows; ++i)
            df.set_value (name, i, d(rng));

    // Write it
    stringstream ss;
    write (ss, df, precision);

    // Read it
    const auto tmp = read (ss);

    // Compare
    VERIFY (df == tmp);
}

void test_timing ()
{
    // Create the dataframe column names
    const size_t cols = 16;
    vector<string> names (cols);
    size_t n = 0;
    generate (names.begin(), names.end(), [&](){return to_string(n++);});

    // Create a dataframe
    dataframe df;
    const size_t rows = 100'000;
    for (const auto &i : names)
        df.add_column(i);
    df.set_rows (rows);

    // Set the data to random numbers
    mt19937 rng(12345);
    std::uniform_real_distribution<> d(1.0, 100.0);
    for (const auto &name : df.get_headers ())
        for (size_t i = 0; i < rows; ++i)
            df.set_value (name, i, d(rng));

    temp_file tf;
    timer t0;
    clog << "Writing " << rows << " rows to " << tf.name << endl;
    timer t1;
    write (tf.name, df);
    t1.stop ();
    clog << "Reading " << rows << " rows from " << tf.name << endl;
    timer t2;
    const auto tmp = read (tf.name);
    t2.stop ();
    clog << "Write " << t1.elapsed_ms () << "ms" << endl;
    clog << "Read  " << t2.elapsed_ms () << "ms" << endl;
    clog << "Total " << t0.elapsed_ms () << "ms" << endl;
}

int main ()
{
    try
    {
        test_dataframe (1, 1, 16);
        test_dataframe (10, 10, 16);
        test_dataframe (10, 100, 16);
        test_dataframe (100, 1, 16);
        test_timing ();

        return 0;
    }
    catch (const exception &e)
    {
        cerr << e.what () << endl;
        return -1;
    }
}
