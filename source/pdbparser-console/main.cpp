
#include <iostream>
#include <fstream>
#include <string>
#include "optionparser.h"
#include <memory>

#include <template-version.h>
#include <symenglib/symenglib_api.h>
#include <symenglib/msf.h>

enum optionIndex {
    pdbparser, HELP, VERBOSE
};

const option::Descriptor usage[] = {
    {pdbparser, 0, "", "", option::Arg::None, "USAGE: pdbparser-console [options]\n\n" "Options:"},
    { VERBOSE, 0, "v", "verbose", option::Arg::None, 0}, // VERBOSE (counted option)
    {HELP, 0, "h", "help", option::Arg::None, "  --help  \tPrint usage and exit."},
    {pdbparser, 0, "", "", option::Arg::None, "\nExamples:\n"
        " pdbparser-console [path-to-pdb-file] \n"
        " "},
    {0, 0, 0, 0, 0, 0}
};

int main(int argc, char* argv[]) {
    argc -= (argc > 0);
    argv += (argc > 0); // skip program name argv[0] if present
    option::Stats stats(usage, argc, argv);
    option::Option* options = new option::Option[stats.options_max];
    option::Option* buffer = new option::Option[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    if (parse.error())
        return 1;

    if (options[VERBOSE]) {
        std::cout << "Version: " << TEMPLATE_VERSION << std::endl;
        std::cout << "pdbparser_GIT_SHA1: " << TEMPLATE_GIT_SHA1 << std::endl;
        std::cout << "pdbparser_GIT_REV: " << TEMPLATE_GIT_REV << std::endl;
        return 0;
    }

    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }
    
    for (int i = 0; i < parse.nonOptionsCount(); ++i)
        std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";
    
    std::shared_ptr<symenglib::MSF> msf(new symenglib::MSF());
    
    
    
    
    
    delete[] options;
    delete[] buffer;
}
