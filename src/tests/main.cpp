#include "tests/core/arm/Disassembly.hpp"

#include <iostream>


int main(int argc, char *argv[]) {
    return lest::run(arm_disassembly_tests, argc, argv);
}