#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"

int main() {
    auto arr = array_load_from_file("input/1"_arr);
    s64 sum = 0;
    s64 first = -1, last = -1;
    Array_t<Array_t<u8>> digits = {
        "one"_arr, "two"_arr, "three"_arr, "four"_arr, "five"_arr,
        "six"_arr, "seven"_arr, "eight"_arr, "nine"_arr
    };
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if (c == '\n') {
            sum += 10*first + last;
            first = -1;
            last = -1;
            continue;
        } 
        
        s64 val = -1;
        if ('0' <= c and c <= '9') {
            val = c - '0';
        } else {
            for (s64 j = 0; j < digits.size; ++j) {
                auto digit = digits[j];
                if (not array_startswith(array_subarray(arr, i), digit)) continue;
                val = j+1;
                break;
            }
        }
        if (val == -1) continue;
        if (first == -1) first = val;
        last = val;
    }
    format_print("%d\n", sum);
}