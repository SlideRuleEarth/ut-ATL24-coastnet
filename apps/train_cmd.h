#ifndef CMD_H
#define CMD_H

#include "cmd_utils.h"

namespace ATL24_coastnet
{

namespace cmd
{

struct args
{
    bool help = false;
    bool verbose = false;
    size_t random_seed = 123;
    std::string model_filename = std::string ("./coastnet_model.pt");
    double train_test_split = 0.0;
    size_t epochs = 20;
    size_t test_dataset = 0;
    size_t num_classes = 5;
};

std::ostream &operator<< (std::ostream &os, const args &args)
{
    os << std::boolalpha;
    os << "help: " << args.help << std::endl;
    os << "verbose: " << args.verbose << std::endl;
    os << "random-seed: " << args.random_seed << std::endl;
    os << "model-filename: " << args.model_filename << std::endl;
    os << "train-test-split: " << args.train_test_split << std::endl;
    os << "epochs: " << args.epochs << std::endl;
    os << "test-dataset: " << args.test_dataset << std::endl;
    os << "num-classes: " << args.num_classes << std::endl;
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
            {"model-filename", required_argument, 0,  'f' },
            {"train-test-split", required_argument, 0,  't' },
            {"epochs", required_argument, 0,  'e' },
            {"test-dataset", required_argument, 0,  'd' },
            {"num-classes", required_argument, 0,  'c' },
            {0,      0,           0,  0 }
        };

        int c = getopt_long(argc, argv, "hvs:f:t:e:d:c:", long_options, &option_index);
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
            case 'f': args.model_filename = std::string(optarg); break;
            case 't': args.train_test_split = atof(optarg); break;
            case 'e': args.epochs = atol(optarg); break;
            case 'd': args.test_dataset = atol(optarg); break;
            case 'c': args.num_classes = atol(optarg); break;
        }
    }

    // Check command line
    if (optind != argc)
        throw std::runtime_error ("Too many arguments on command line");

    // Check args
    if (args.train_test_split < 0.0)
        throw std::runtime_error ("train-test-split must be >= 0.0");
    if (args.train_test_split > 0.5)
        throw std::runtime_error ("train-test-split must be <= 0.5");

    const size_t total_datasets = ((args.train_test_split == 0.0)
        ? 1
        : std::ceil (1.0 / args.train_test_split));

    if (args.test_dataset >= total_datasets)
    {
        std::clog << "Total datasets = " << total_datasets << std::endl;
        std::clog << "Test dataset = " << args.test_dataset << std::endl;
        throw std::runtime_error ("Test dataset specified is greater than the total");
    }

    return args;
}

} // namespace cmd

} // namespace ATL24_coastnet

#endif // CMD_H
