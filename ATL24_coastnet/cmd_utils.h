#ifndef CMD_UTILS_H
#define CMD_UTILS_H

#include "precompiled.h"

namespace ATL24_coastnet
{

namespace cmd
{

void print_help (std::ostream &os, const std::string &usage, const size_t noptions, option long_options[])
{
    // Print usage string
    os << "Usage:" << std::endl << '\t' << usage << std::endl << std::endl;

    // Print associated options, if any
    if (noptions == 0)
        return;

    os << "Options:" << std::endl;

    for (size_t i = 0; i + 1 < noptions; ++i)
    {
        os << "\t--" << long_options[i].name;
        if (long_options[i].has_arg)
            os << "=<ARG>";
        os << ", ";
        os << "-" << char (long_options[i].val);
        if (long_options[i].has_arg)
            os << " <ARG>";
        os << std::endl;
    }
}

} // namespace cmd

} // namespace ATL24_coastnet

#endif // CMD_UTILS_H
