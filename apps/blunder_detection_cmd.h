#ifndef CMD_H
#define CMD_H

#include <cmath>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "ATL24_coastnet/cmd_utils.h"

namespace ATL24_coastnet
{

namespace cmd
{

struct args
{
    bool help = false;
    bool verbose = false;
    double surface_min_elevation = NAN;
    double surface_max_elevation = NAN;
    double bathy_min_elevation = NAN;
    double water_column_width = NAN;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "surface_min_elevation: " << args.surface_min_elevation << std::endl;
    os << "surface_max_elevation: " << args.surface_max_elevation << std::endl;
    os << "bathy_min_elevation: " << args.bathy_min_elevation << std::endl;
    os << "water_column_width: " << args.water_column_width << std::endl;
    return os;
}

args get_args (int argc, char **argv, const std::string &usage)
{
    using namespace std;

    args args;
    while (1)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0,  'h' },
            {"verbose", no_argument, 0,  'v' },
            {"surface-min-elevation", required_argument, 0,  'n' },
            {"surface-max-elevation", required_argument, 0,  'x' },
            {"bathy-min-elevation", required_argument, 0,  'b' },
            {"water-column-width", required_argument, 0,  'w' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvn:x:b:w:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            default:
            case 0:
            case 'h':
            {
                const size_t noptions = sizeof (long_options) / sizeof (struct option);
                cmd::print_help (std::clog, usage, noptions, long_options);
                if (c != 'h')
                    throw std::runtime_error ("Invalid option");
                args.help = true;
                return args;
            }
            case 'v': args.verbose = true; break;
            case 'n': args.surface_min_elevation = atof(optarg); break;
            case 'x': args.surface_max_elevation = atof(optarg); break;
            case 'b': args.bathy_min_elevation = atof(optarg); break;
            case 'w': args.water_column_width = atof(optarg); break;
        }
    }

    // Check command line
    if (optind != argc)
        throw std::runtime_error ("Too many arguments on command line");

    return args;
}

} // namespace cmd

} // namespace ATL24_coastnet

#endif // CMD_H
