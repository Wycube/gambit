#pragma once

#include "Types.hpp"

#include <type_traits>
#include <cstring>


namespace common {

template<typename T, typename = std::enable_if<std::is_integral<T>::value>>
auto generate_exclusion_mask(const char *pattern, size_t &j, size_t end) -> T {
    T mask = 0;
    
    do {
        char c = pattern[j];

        if(c != '<' && c != '>') {
            j--;
            break;
        }
        
        //OR a 1 if testing against all ones (<), and a zero for all zeros (>)
        mask |= (c == '>') << j;
    } while(++j < end);

    return mask;
}

/* 
 * x means 1 or 0
 * < means can't all be unset
 * > means can't all be set
 * i.e. << can be 01, 10, 11, but not 00
 * and >> can be 00, 01, 10, but not 11
 * Only 12 bits are needed to decode a 32-bit ARM instruction
 * bits 27-20 (8) + bits 7-4 (4).
 * TODO: Add tests for this
 */
template<typename T, typename = std::enable_if<std::is_integral<T>::value>, size_t _count>
auto match_bits(T value, const char *(&patterns)[_count]) -> size_t {
    for(size_t i = 0; i < _count; i++) {
        size_t length = std::min(sizeof(T) * 8, std::strlen(patterns[i]));
        const char *pattern = patterns[i];
        bool match = true;

        for(size_t j = 0; j < length; j++) {
            u8 bit = (value >> (length - 1 - j)) & 0x1;
            T mask, temp = value;
            size_t _j = j;

            switch(pattern[j]) {
                case 'x' : continue;

                case '0' : match = bit == 0;
                break;
                case '1' : match = bit == 1;
                break;

                case '<' :
                case '>' :
                    mask = generate_exclusion_mask<T>(pattern, j, length);
                    temp &= ((1 << (j - _j + 1)) - 1) << (length - 1 - _j); //Mask out all bits not being tested
                    match = ((temp & mask) != mask || mask == 0) && ((temp & ~mask) != 0); //Test if the bits match the pattern, with some bitwise magic
                break;
            }

            if(!match) { break; }
        }

        if(match) { return i; }
    }

    return _count;
}

} //namespace common