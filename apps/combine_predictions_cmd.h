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
    double surface_sigma = 40.0;
    double bathy_sigma = 40.0;
    std::string fn1;
    std::string fn2;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "surface_sigma: " << args.surface_sigma << std::endl;
    os << "bathy_sigma: " << args.bathy_sigma << std::endl;
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
            {"surface-sigma", required_argument, 0,  's' },
            {"bathy-sigma", required_argument, 0,  'b' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvs:b:", long_options, &option_index);
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
            case 's': args.surface_sigma = atof(optarg); break;
            case 'b': args.bathy_sigma = atof(optarg); break;
        }
    }

    // Get remaining arguments
    if (optind == argc)
        throw std::runtime_error ("Not enough arguments on command line");

    args.fn1 = string(argv[optind++]);

    if (optind == argc)
        throw std::runtime_error ("Not enough arguments on command line");

    args.fn2 = string(argv[optind++]);

    if (optind != argc)
        throw std::runtime_error ("Too many arguments on command line");

    return args;
}

} // namespace cmd

} // namespace ATL24_coastnet

#endif // CMD_H
