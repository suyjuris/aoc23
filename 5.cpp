#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

void parse_int_line(Array_t<u8> text, s64* inout_index, Array_dyn<s64>* out) {
    s64 number = 0;
    s64 state = 0;
    s64 i;
    for (i = *inout_index; i < text.size; ++i) {
        if ('0' <= c and c <= '9') {
            number = 10*number + (c - '0');
            state = 1;
        } else if (state == 1) {
            array_push_back(out, number);
            state = 0;
            number = 0;
        }
        if (c == '\n') {
            ++i;
            break;
        }
    }
    *inout_index = i;
}

int main() {
    auto arr = array_load_from_file("input/4"_arr);
    
    s64 index = 0;
    Array_dyn<s64> seeds;
    parse_int_line(arr, &index, &seeds);
    Array_dyn<s64> seeds_new;
    Array_dyn<s64> mapping;
    
    for (; index < arr.size; ++index) {
        if (arr[index] != ':') continue;
        
        index += 2;
        while (arr[index] != '\n') {
            parse_int_line(arr, &index, &mapping);
            
        }
    }
    
    format_print("%d\n", sum);
}