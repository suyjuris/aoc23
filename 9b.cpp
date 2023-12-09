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
    bool sign = false;
    for (i = *inout_index; i < text.size; ++i) {
        u8 c = text[i];
        if ('0' <= c and c <= '9') {
            number = 10*number + (c - '0');
            state = 1;
        } else if (c == '-') {
            sign ^= 1;
        } else if (state == 1) {
            array_push_back(out, sign ? -number : number);
            state = 0;
            number = 0;
            sign = false;
        }
        if (c == '\n') {
            ++i;
            break;
        }
    }
    *inout_index = i;
}


int main() {
    auto arr = array_load_from_file("input/9"_arr);
    
    Array_dyn<Array_dyn<s64>> numbers;
    
    s64 index = 0;
    s64 sum = 0;
    while (index < arr.size) {
        array_push_back(&numbers, {});
        parse_int_line(arr, &index, &numbers[0]);
        
        while (true) {
            bool flag = false;
            auto a = numbers.back();
            for (s64 i: a) {
                flag |= i != 0;
            }
            if (not flag) break;
            
            array_push_back(&numbers, {});
            auto& b = numbers.back();
            
            for (s64 i = 0; i+1 < a.size; ++i) {
                array_push_back(&b, a[i+1] - a[i]);
            }
        }
        
        for (s64 i = numbers.size-2; i >= 0; --i) {
            array_push_back(&numbers[i], numbers[i][0] - numbers[i+1].back());
        }
        sum += numbers[0].back();
        
        for (auto& a: numbers) {
            array_free(&a);
        }
        numbers.size = 0;
    }
    
    format_print("%d\n", sum);
}