#include "tests/core/arm/Disassembly.hpp"

#include <iostream>


int main(int argc, char *argv[]) {
    int failures = lest::run(arm_disassembly_tests, argc, argv);
    std::cout << "\n" << failures << " test(s) failed!\n";

    return failures;
}