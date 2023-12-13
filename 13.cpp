#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

s64 find_mirror(Array_t<u64> hashes) {
    for (s64 i = 0; i+1 < hashes.size; ++i) {
        bool reflection = true;
        for (s64 j = 0; i-j >= 0 and i+1+j < hashes.size; ++j) {
            if (hashes[i-j] != hashes[i+1+j]) reflection = false;
        }
        if (reflection) {
            return i+1;
        }
    }
    return -1;
}

int main() {
    auto arr = array_load_from_file("input/13"_arr);
    
    Array_dyn<u64> hashes;
    
    s64 index = 0;
    s64 sum = 0;
    while (index < arr.size) {
        s64 start = index;
        
        s64 width;
        {
            s64 i = index;
            while (arr[i] != '\n') ++i;
            width = i - index;
            ++i;
        }
        
        s64 height = 0;
        while (index < arr.size and arr[index] != '\n') {
            height += 1;
            index += width + 1;
        }
        
        index += 1;
        
        auto block = array_subarray(arr, start, start + (width+1)*height);
        
        // Columns
        hashes.size = 0;
        for (s64 x = 0; x < width; ++x) {
            u64 h = 0;
            for (s64 y = 0; y < height; ++y) {
                h = hash_u64_pair(h, block[x + (width+1)*y]);
            }
            array_push_back(&hashes, h);
        }
        s64 mirror = find_mirror(hashes);
        if (mirror != -1) {
            sum += mirror;
            continue;
        }
        
        // Rows
        hashes.size = 0;
        for (s64 y = 0; y < height; ++y) {
            u64 h = 0;
            for (s64 x = 0; x < width; ++x) {
                h = hash_u64_pair(h, block[x + (width+1)*y]);
            }
            array_push_back(&hashes, h);
        }
        mirror = find_mirror(hashes);
        if (mirror != -1) {
            sum += mirror * 100;
            continue;
        }
        
        assert(false);
    }
    
    format_print("%d\n", sum);
}
