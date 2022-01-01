#pragma once

#include "Types.hpp"
#include "Log.hpp"

#include <type_traits>
#include <array>
#include <cstring>


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
 * Seperated angle brackets act together.
 */
template<typename T, typename = std::enable_if<std::is_integral<T>::value>, size_t _count>
auto match_bits(T value, const char *(&patterns)[_count]) -> size_t {
    for(size_t i = 0; i < _count; i++) {
        size_t length = std::strlen(patterns[i]);
        
        //Skip if pattern is bigger than the number of bits in T
        if(length > sizeof(T) * 8) {
            continue;
        }

        const char *pattern = patterns[i];
        bool match = true;
        bool excl_mask_used = false;

        for(size_t j = 0; j < length; j++) {
            u8 bit = (value >> (length - 1 - j)) & 0x1;
            std::pair<T, T> mask;
            T temp = value;

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
                    //Mask out all bits not being tested
                    temp &= mask.second;
                    //Test if the bits match the exclusion pattern, with an XOR
                    match = (temp ^ mask.first) != 0;
                    excl_mask_used = true;
                break;
            }

            if(!match) break;
        }

        if(match) return i;
    }

    return _count;
}


//Bit Pattern Matching via generating certain bitmasks at compile-time

template<typename T, typename = std::enable_if<std::is_integral<T>::value>>
struct PatternMask {
    T exclusion_bits, exclusion_mask, constant_mask, result;

    constexpr PatternMask() : exclusion_bits(0), exclusion_mask(0), constant_mask(0), result(0) { }
};

template<typename T, typename = std::enable_if<std::is_integral<T>::value>, size_t _length>
constexpr auto generate_pattern_mask(const char (&pattern)[_length]) -> PatternMask<T> {
    //Patterns cannot be bigger than the size of the integral type
    static_assert(sizeof(T) * 8 >= _length - 1);

    PatternMask<T> masks;
    
    for(size_t i = 0; i < _length - 1; i++) {
        char c = pattern[i];
        
        switch(c) {
            case 'x' : continue;

            case '0' :
            case '1' : 
                masks.constant_mask |= (1 << (_length - 2 - i));
                masks.result |= ((c == '1') << (_length - 2 - i));
                break;

            case '<' :
            case '>' :
                masks.exclusion_bits |= (1 << (_length - 2 - i));
                masks.exclusion_mask |= ((c == '>') << (_length - 2 - i));
                break;
        }
    }

    return masks;
}

template<size_t _count, size_t _length, const char patterns[_count][_length], typename T, typename = std::enable_if<std::is_integral<T>::value>>
constexpr auto generate_multiple_masks() -> std::array<PatternMask<T>, _count> {
    std::array<PatternMask<T>, _count> pattern_masks;

    for(size_t i = 0; i < _count; i++) {
        pattern_masks[i] = generate_pattern_mask<T>(patterns[i]);
    }

    return pattern_masks;
}

template<size_t _count, size_t _length, const char patterns[_count][_length], typename T, typename = std::enable_if<std::is_integral<T>::value>>
auto const_match_bits(T value) -> size_t {
    static constexpr std::array<PatternMask<T>, _count> pattern_masks = generate_multiple_masks<_count, _length, patterns, T>();

    for(size_t i = 0; i < _count; i++) {
        const PatternMask<T> &pattern = pattern_masks[i];
        T excl_bits = value & pattern.exclusion_bits;

        if((value & pattern.constant_mask) == pattern.result) {
            if(pattern.exclusion_bits != 0 && (excl_bits ^ pattern.exclusion_mask) == 0) {
                continue;
            }

            return i;
        }
    }

    return _count;
}

} //namespace common