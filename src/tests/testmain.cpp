#include "tests/Tests.hpp"

#include <iostream>


int main(int argc, char *argv[]) {
    //For command line arguments
    lest::run(lest::tests(), argc, argv);
    
    int failures = 0;
    for(lest::tests t : all_tests) {
        failures += lest::run(t, 1, argv);
    }

    if(failures != 0)
        std::cout << "\n" << failures << " test(s) failed!\n";
    else
        std::cout << "\nAll tests passed!\n";

    return failures;
}