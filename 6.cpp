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
        u8 c = text[i];
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
    auto arr = array_load_from_file("input/6"_arr);
    
    s64 index = 0;
    Array_dyn<s64> durations, distances;
    parse_int_line(arr, &index, &durations);
    parse_int_line(arr, &index, &distances);
    
    assert(durations.size == distances.size);
    
    s64 prod = 1;
    for (s64 i = 0; i < durations.size; ++i) {
        double t = durations[i];
        double d = distances[i];
        
        double x0 = (t-sqrt(t*t-4*d)) / 2;
        double x1 = (t+sqrt(t*t-4*d)) / 2;
        
        if (x0 < 0) x0 = 0;
        if (x1 < 0) x1 = 0;
        s64 count = floor(nextafter(x1, x1-1)) - ceil(nextafter(x0, x0+1)) + 1;
        prod *= count;
    }
    
    format_print("%d\n", prod);
}