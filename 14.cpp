#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/14"_arr);
    
    s64 width = -1;
    for (s64 i = 0; i < arr.size; ++i) {
        if (arr[i] == '\n') {
            width = i;
            break;
        }
    }
    assert(width != -1);
    assert(arr.size % (width + 1) == 0);
    s64 height = arr.size / (width + 1);
    
    s64 sum = 0;
    Array_dyn<u8> col;
    for (s64 x = 0; x < width; ++x) {
        col.size = 0;
        for (s64 y = 0; y < height; ++y) {
            array_push_back(&col, arr[x + y*(width+1)]);
        }
        
        s64 last = 0;
        for (s64 i = 0; i < col.size; ++i) {
            if (col[i] == 'O') {
                if (last != i) {
                    assert(col[last] == '.');
                    col[last] = 'O';
                    col[i] = '.';
                }
                last = last+1;
            } else if (col[i] == '#') {
                last = i+1;
            }
        }
        for (s64 i = 0; i < col.size; ++i) {
            if (col[i] == 'O') sum += height - i;
        }
    }
    
    format_print("%d\n", sum);
}
