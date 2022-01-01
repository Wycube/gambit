#pragma once

#include "Types.hpp"

#include <string>


namespace common {

inline auto hex(u32 value) -> std::string {
    //Check the last seven nibbles to see if any can be removed (they are zero)
    u8 start = 7;
    for(int i = 7; i > 0; i--) {
        if(((value >> i * 4) & 0xF) == 0) {
            start--;
        } else {
            break;
        }
    }

    //Loop through the one nibble at a time and translate to a hex digit
    std::string hex;
    
    for(int i = start; i >= 0; i--) {
        u8 digit = (value >> i * 4) & 0xF;
        hex += digit >= 10 ? 'a' + (digit - 10) : '0' + digit;
    }

    return hex;
}

inline auto alphabetic(char c) -> bool {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

inline auto to_upper(const std::string &text) -> std::string {
    std::string capitalized = text;
    
    for(int i = 0; i < capitalized.size(); i++) {
        char c = capitalized[i];
        
        if('a' <= c && c <= 'z') {
            capitalized[i] = 'A' + (capitalized[i] - 'a');
        }
    }

    return capitalized;
}

inline auto to_lower(const std::string &text) -> std::string {
    std::string lower_case = text;
    
    for(int i = 0; i < lower_case.size(); i++) {
        char c = lower_case[i];

        if('A' <= c && c <= 'Z') {
            lower_case[i] = 'a' + (c - 'A');
        }
    }

    return lower_case;
}

inline auto is_printable(char c) -> bool {
    return c >= 32 && c <= 126;
}

} //namespace common