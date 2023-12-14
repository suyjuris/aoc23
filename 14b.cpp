#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

void roll(Array_dyn<u8>& col, Array_t<u8> arr, u8 dir, s64 width, s64 height) {
    s64 x_size = dir % 2 ? height : width;
    s64 y_size = dir % 2 ? width : height;
    for (s64 x = 0; x < x_size; ++x) {
        col.size = 0;
        for (s64 y = 0; y < y_size; ++y) {
            s64 index = -1;
            switch (dir) {
                case 0: index = x + y*(width+1); break;
                case 1: index = y + x*(width+1); break;
                case 2: index = x + (y_size-1 - y)*(width+1); break;
                case 3: index = (y_size-1 - y) + x*(width+1); break;
            }
            array_push_back(&col, arr[index]);
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
        for (s64 y = 0; y < col.size; ++y) {
            s64 index = -1;
            switch (dir) {
                case 0: index = x + y*(width+1); break;
                case 1: index = y + x*(width+1); break;
                case 2: index = x + (y_size-1 - y)*(width+1); break;
                case 3: index = (y_size-1 - y) + x*(width+1); break;
            }
            arr[index] = col[y];
        }
    }
}

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
    
    Array_dyn<u8> col;
    
    s64 count = 0;
    Hashmap<s64> cache;
    cache.empty = -1;
    s64 remaining = 1000000000;
    bool final = false;
    while (remaining > 0) {
        for (u8 d = 0; d < 4; ++d) {
            roll(col, arr, d, width, height);
        }
        --remaining;
        ++count;
        if (final) continue;
        
        u64 h = hash_str(arr);
        s64* ptr = hashmap_getcreate(&cache, h, count);
        if (*ptr == count) continue;
        
        s64 period = count - *ptr;
        remaining %= period;
        final = true;
    }
    
    s64 sum = 0;
    for (s64 x = 0; x < width; ++x) {
        for (s64 y = 0; y < height; ++y) {
            if (arr[x + y*(width+1)] == 'O') sum += height - y;
        }
    }
    format_print("%d\n", sum);
}
