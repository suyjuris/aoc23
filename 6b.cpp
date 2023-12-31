#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

void parse_one_int_line(Array_t<u8> text, s64* inout_index, s64* out) {
    s64 number = 0;
    s64 i;
    for (i = *inout_index; i < text.size; ++i) {
        u8 c = text[i];
        if ('0' <= c and c <= '9') {
            number = 10*number + (c - '0');
        } else if (c == '\n') {
            *out = number;
            ++i;
            break;
        }
    }
    *inout_index = i;
}
int main() {
    auto arr = array_load_from_file("input/6"_arr);
    
    s64 index = 0;
    s64 duration, distance;
    parse_one_int_line(arr, &index, &duration);
    parse_one_int_line(arr, &index, &distance);
    
    double t = duration;
    double d = distance;
    
    double x0 = (t-sqrt(t*t-4*d)) / 2;
    double x1 = (t+sqrt(t*t-4*d)) / 2;
    
    if (x0 < 0) x0 = 0;
    if (x1 < 0) x1 = 0;
    s64 count = floor(nextafter(x1, x1-1)) - ceil(nextafter(x0, x0+1)) + 1;
    
    format_print("%d\n", count);
}