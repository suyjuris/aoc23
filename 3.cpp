#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"

int main() {
    auto arr = array_load_from_file("input/3"_arr);
    s64 sum = 0;
    
    s64 width = -1;
    for (s64 i = 0; i < arr.size; ++i) {
        if (arr[i] == '\n') {
            width = i+1;
            break;
        }
    }
    assert(width != -1 and arr.size % width == 0);
    
    s64 n_lines = arr.size / width;
    auto get = [&arr, width, n_lines](s64 x, s64 y) {
        if (0 <= x and x <= width and 0 <= y and y < n_lines) {
            return arr[y * width + x];
        } else {
            return (u8)'.';
        }
    };
    
    s64 number = 0;
    s64 number_begin = -1;
    
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if ('0' <= c and c <= '9') {
            number = 10*number + (c - '0');
            if (number_begin == -1) number_begin = i;
        } else if (number_begin != -1) {
            s64 w = i - number_begin;
            s64 ix = number_begin % width;
            s64 iy = number_begin / width;
            bool found = false;
            for (s64 y = iy-1; y <= iy+1; ++y) {
                for (s64 x = ix-1; x < ix+w+1; ++x) {
                    u8 cxy = get(x, y);
                    if (cxy != '.' and not ('0' <= cxy and cxy <= '9') and cxy != '\n') {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (found) sum += number;
            
            number_begin = -1;
            number = 0;
        }
    }
    format_print("%d\n", sum);
}