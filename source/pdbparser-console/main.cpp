
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <boost/program_options.hpp>

#include <template-version.h>
#include <symenglib/symenglib_api.h>
#include <symenglib/msf.h>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    po::options_description desc("General options");

    desc.add_options()
            ("v,version", "Show version")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("v")) {
        std::cout << "Version: " << TEMPLATE_VERSION << std::endl;
        std::cout << "pdbparser_GIT_SHA1: " << TEMPLATE_GIT_SHA1 << std::endl;
        std::cout << "pdbparser_GIT_REV: " << TEMPLATE_GIT_REV << std::endl;
        return 1;
    }
    
     //std::shared_ptr<symenglib::MSF> msf(new symenglib::MSF());


}
