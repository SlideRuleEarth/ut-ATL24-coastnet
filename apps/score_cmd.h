#pragma once

#include "ATL24_coastnet/precompiled.h"
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
    std::string prediction_label;
    std::string csv_filename;
    int ignore_cls = -1;
    std::vector<std::string> filenames;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "class: " << args.cls << std::endl;
    os << "prediction-label: '" << args.prediction_label << "'" << std::endl;
    os << "csv-filename: '" << args.csv_filename << "'" << std::endl;
    os << "ignore-class: " << args.ignore_cls << std::endl;
    os << "filenames: " << args.filenames.size () << " total" << std::endl;
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
            {"prediction-label", required_argument, 0,  'l' },
            {"csv-filename", required_argument, 0,  's' },
            {"ignore-class", required_argument, 0,  'i' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvc:l:s:i:", long_options, &option_index);
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
            case 'l': args.prediction_label = std::string(optarg); break;
            case 's': args.csv_filename = std::string(optarg); break;
            case 'i': args.ignore_cls = atol(optarg); break;
        }
    }

    // Check command line
    assert (optind <= argc);
    while (optind != argc)
        args.filenames.push_back (argv[optind++]);

    return args;
}

} // namespace cmd

} // namespace ATL24_coastnet
