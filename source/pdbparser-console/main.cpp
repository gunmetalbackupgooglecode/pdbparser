
#include <iostream>
#include <fstream>
#include <string>
#include "optionparser.h"

#include <template-version.h>

enum optionIndex {
    UNKNOWN, HELP
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, "USAGE: pdbparser-console [options]\n\n"
        "Options:"},
    {HELP, 0, "", "help", option::Arg::None, "  --help  \tPrint usage and exit."},
    {UNKNOWN, 0, "", "", option::Arg::None, "\nExamples:\n"
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

    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }

    std::cout << "Version: " << TEMPLATE_VERSION << std::endl;
    std::cout << "pdbparser_GIT_SHA1: " << TEMPLATE_GIT_SHA1 << std::endl;




    delete[] options;
    delete[] buffer;
}
