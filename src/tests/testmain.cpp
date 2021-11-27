#include "tests/core/arm/Disassembly.hpp"

#include <iostream>


int main(int argc, char *argv[]) {
    int failures = lest::run(arm_disassembly_tests, argc, argv);

    if(failures != 0)
        std::cout << "\n" << failures << " test(s) failed!\n";
    else
        std::cout << "\nAll tests passed!\n";

    return failures;
}