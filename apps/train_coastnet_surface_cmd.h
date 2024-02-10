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
    size_t total_samples_per_class = 1'000;
    size_t epochs = 10;
    size_t batch_size = 0;
    std::string network_filename;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "random-seed: " << args.random_seed << std::endl;
    os << "total-samples-per-class: " << args.total_samples_per_class << std::endl;
    os << "epochs: " << args.epochs << std::endl;
    os << "batch-size: " << args.batch_size << std::endl;
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
            {"total-samples-per-class", required_argument, 0,  't' },
            {"epochs", required_argument, 0,  'e' },
            {"batch-size", required_argument, 0,  'b' },
            {"network-filename", required_argument, 0,  'f' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvs:t:e:b:f:", long_options, &option_index);
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
            case 't': args.total_samples_per_class = std::atol(optarg); break;
            case 'e': args.epochs = std::atol(optarg); break;
            case 'b': args.batch_size = std::atol(optarg); break;
            case 'f': args.network_filename = std::string(optarg); break;
        }
    }

    // Check command line
    if (optind != argc)
        throw std::runtime_error ("Too many arguments on command line");

    // Check args
    if (args.network_filename.empty ())
        throw std::runtime_error ("No network filename was specified");

    return args;
}

} // namespace cmd

} // namespace ATL24_resnet

#endif // CMD_H
