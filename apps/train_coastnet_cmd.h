#ifndef CMD_H
#define CMD_H

#include <getopt.h>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <string>
#include "ATL24_resnet/cmd_utils.h"

namespace ATL24_resnet
{

namespace cmd
{

struct args
{
    bool help = false;
    bool verbose = false;
    size_t random_seed = 123;
    unsigned cls = 0;
    size_t max_samples_per_class = 1'000;
    std::string network_filename;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "random-seed: " << args.random_seed << std::endl;
    os << "class: " << args.cls << std::endl;
    os << "max-samples-per-class: " << args.max_samples_per_class << std::endl;
    os << "network-filename: " << args.network_filename << std::endl;
    return os;
}

args get_args (int argc, char **argv, const std::string &usage)
{
    args args;
    while (1)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0,  'h' },
            {"verbose", no_argument, 0,  'v' },
            {"random-seed", required_argument, 0,  's' },
            {"class", required_argument, 0,  'c' },
            {"max-samples-per-class", required_argument, 0,  'm' },
            {"network-filename", required_argument, 0,  'f' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvs:c:m:f:", long_options, &option_index);
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
            case 's': args.random_seed = atol(optarg); break;
            case 'c': args.cls = std::atol(optarg); break;
            case 'm': args.max_samples_per_class = std::atol(optarg); break;
            case 'f': args.network_filename = std::string(optarg); break;
        }
    }

    // Check command line
    if (optind != argc)
        throw std::runtime_error ("Too many arguments on command line");

    // Check args
    if (args.network_filename.empty ())
        throw std::runtime_error ("No network filename was specified");

    // Check args
    if (args.cls == 0)
        throw std::runtime_error ("You did not specify a class");

    return args;
}

} // namespace cmd

} // namespace ATL24_resnet

#endif // CMD_H
