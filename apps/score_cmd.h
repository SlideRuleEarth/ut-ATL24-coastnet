#ifndef CMD_H
#define CMD_H

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
    int cls = -1;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "class: " << args.cls << std::endl;
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
            {"class", required_argument, 0,  'c' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvc:", long_options, &option_index);
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
            case 'c': args.cls = atol(optarg); break;
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
