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

Hashmap<s64> cache;

s64 count_possibilities(Array_t<u8> springs, Array_t<s64> groups) {
    u64 key = springs.size << 32 | groups.size;
    if (s64* ptr = hashmap_getptr(&cache, key)) {
        return *ptr;
    }
    
    if (groups.size == 0) {
        for (u8 c: springs) {
            if (c == '#') {
                return 0;
            }
        }
        return 1;
    }
    
    s64 n = groups[0];
    s64 count = 0;
    bool notlast = groups.size > 1;
    for (s64 i = 0; i + n + notlast <= springs.size; ++i) {
        bool possible = true;
        for (s64 j = i; j < i+n; ++j) {
            if (springs[j] == '.') possible = false;
        }
        if (notlast and springs[i+n] == '#') possible = false;
        if (possible) {
            count += count_possibilities(array_subarray(springs, i+n+notlast), array_subarray(groups, 1));
        }
        
        if (springs[i] == '#') break;
    }
    
    hashmap_set(&cache, key, count);
    return count;
}

int main() {
    auto arr = array_load_from_file("input/12"_arr);
    
    Array_dyn<u8> springs_expanded;
    Array_dyn<s64> groups;
    Array_dyn<s64> groups_expanded;
    
    cache.empty = -1;
    
    s64 index = 0;
    s64 sum = 0;
    while (index < arr.size) {
        s64 line_start = index;
        while (arr[index] != ' ') ++index;
        auto springs = array_subarray(arr, line_start, index);
        groups.size = 0;
        parse_int_line(arr, &index, &groups);
        
        springs_expanded.size = 0;
        groups_expanded.size = 0;
        for (s64 i = 0; i < 5; ++i) {
            if (i) array_push_back(&springs_expanded, '?');
            array_append(&springs_expanded, springs);
            array_append(&groups_expanded, groups);
        }
        
        hashmap_clear(&cache);
        s64 count = count_possibilities(springs_expanded, groups_expanded);
        //format_print("%d\n", count);
        sum += count;
    }
    
    format_print("%d\n", sum);
}
