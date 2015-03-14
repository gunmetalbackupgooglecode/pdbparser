
#include <iostream>
#include <fstream>
#include <string>

#include <template-version.h>

int main(int /*argc*/, char* /*argv*/[])
{
    std::cout << "Version: " << TEMPLATE_VERSION << std::endl;
    return 0;
} 
