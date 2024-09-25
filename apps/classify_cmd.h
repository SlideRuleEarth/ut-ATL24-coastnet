#ifndef CMD_H
#define CMD_H

#include "../ATL24_coastnet/cmd_utils.h"

namespace ATL24_coastnet
{

namespace cmd
{

struct args
{
    bool help = false;
    bool verbose = false;
    bool use_predictions = false;
    bool get_results = false;
    size_t num_classes = 5;
    std::string model_filename = std::string ("./coastnet_model.pt");
    std::string results_filename = std::string ("./coastnet_results.txt");
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "use_predictions: " << args.use_predictions << std::endl;
    os << "get-results: " << args.get_results << std::endl;
    os << "num-classes: " << args.num_classes << std::endl;
    os << "model-filename: " << args.model_filename << std::endl;
    os << "results-filename: " << args.results_filename << std::endl;
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
            {"use-predictions", no_argument, 0,  'p' },
            {"get-results", no_argument, 0,  'g' },
            {"num-classes", required_argument, 0,  'c' },
            {"model-filename", required_argument, 0,  'f' },
            {"results-filename", required_argument, 0,  'r' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvpc:f:r:", long_options, &option_index);
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
            case 'p': args.use_predictions = true; break;
            case 'g': args.get_results = true; break;
            case 'c': args.num_classes = atol(optarg); break;
            case 'f': args.model_filename = std::string(optarg); break;
            case 'r': args.results_filename = std::string(optarg); break;
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
