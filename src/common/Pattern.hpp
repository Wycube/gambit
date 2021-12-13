#pragma once

#include "Types.hpp"

#include <type_traits>
#include <cstring>
#include <iostream>
#include <bitset>


namespace common {

template<typename T, typename = std::enable_if<std::is_integral<T>::value>>
auto generate_exclusion_mask(const char *pattern, size_t index, const size_t length) -> std::pair<T, T> {
    T excl_mask = 0, mask = 0;
    
    do {
        char c = pattern[index];

        if(c != '<' && c != '>') {
            continue;
        }
        mask |= 1 << (length - 1 - index);
        
        //OR a 1 if testing against ones (>), and a 0 for zeros (<)
        excl_mask |= (c == '>') << (length - 1 - index);
    } while(++index < length);

    return std::pair<T, T>(excl_mask, mask);
}

/* 
 * - x means 1 or 0
 * - < means can't all be unset
 * - > means can't all be set
 * i.e. << can be 01, 10, 11, but not 00
 * and >> can be 00, 01, 10, but not 11.
 * Even seperated angle brackets act together.
 * TODO: Add tests for this
 */
template<typename T, typename = std::enable_if<std::is_integral<T>::value>, size_t _count>
auto match_bits(T value, const char *(&patterns)[_count]) -> size_t {
    for(size_t i = 0; i < _count; i++) {
        size_t length = std::min(sizeof(T) * 8, std::strlen(patterns[i]));
        const char *pattern = patterns[i];
        bool match = true;
        bool excl_mask_used = false;

        for(size_t j = 0; j < length; j++) {
            u8 bit = (value >> (length - 1 - j)) & 0x1;
            std::pair<T, T> mask;
            T temp = value;

            std::cout << "j: " << j << " char: " << pattern[j] << "\n";

            switch(pattern[j]) {
                case 'x' : continue;

                case '0' : match = bit == 0;
                break;
                case '1' : match = bit == 1;
                break;

                case '<' :
                case '>' :
                    if(excl_mask_used) break;
                    
                    mask = generate_exclusion_mask<T>(pattern, j, length);

                    std::cout << "       Pattern:     " << pattern << "\n";
                    std::cout << "         Value: " << std::bitset<sizeof(T) * 8>(value) << "\n";
                    std::cout << "Exclusion Mask: " << std::bitset<sizeof(T) * 8>(mask.first) << "\n";
                    std::cout << "          Mask: " << std::bitset<sizeof(T) * 8>(mask.second) << "\n";

                    //Mask out all bits not being tested
                    temp &= mask.second;
                    //Test if the bits match the pattern, with some bitwise magic
                    match = ((temp & mask.first) != mask.first || mask.first == 0) || ((temp & ~mask.first) != 0);
                    excl_mask_used = true;
                break;
            }

            if(!match) break;
        }

        if(match) return i;
    }

    return _count;
}

} //namespace common