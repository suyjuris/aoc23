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

struct Interval {
    s64 start = 0, end = 0;
    
    bool empty() { return start >= end; }
};

Interval intersect(Interval a, Interval b) {
    if (a.start > b.start or (a.start == b.start and a.end < b.end)) simple_swap(&a, &b);
    
    if (a.end <= b.start) return {}; // disjoint
    else if (b.end <= a.end) return b; // b contained in a
    else return {b.start, a.end}; // [(])
}

void _push_nonempty(Array_dyn<Interval>* arr, Interval i) {
    if (i.empty()) return;
    array_push_back(arr, i);
}

void subtract(Interval a, Interval b, Array_dyn<Interval>* out) {
    if (a.end <= b.start or b.end <= a.start) {  // disjoint
        _push_nonempty(out, a);
    } else {
        _push_nonempty(out, {a.start, b.start}); 
        _push_nonempty(out, {b.end, a.end}); 
    }
}


int main() {
    auto arr = array_load_from_file("input/5"_arr);
    
    s64 index = 0;
    Array_dyn<Interval> seeds;
    
    Array_dyn<s64> mapping;
    parse_int_line(arr, &index, &mapping);
    assert(mapping.size % 2 == 0);
    for (s64 i = 0; i+1 < mapping.size; i += 2) {
        array_push_back(&seeds, {mapping[i], mapping[i] + mapping[i+1]});
    }
    
    Array_dyn<Interval> seeds_new;
    
    format_print("A ");
    for (auto q: seeds) {
        format_print("[%d,%d), ", q.start, q.end);
    }
    format_print("\n");
    
    for (; index < arr.size; ++index) {
        if (arr[index] != ':') continue;
        
        index += 2;
        while (index < arr.size and arr[index] != '\n') {
            mapping.size = 0;
            parse_int_line(arr, &index, &mapping);
            
            assert(mapping.size == 3);
            Interval source {mapping[1], mapping[1] + mapping[2]};
            s64 diff = mapping[0] - mapping[1];
            
            for (s64 i = 0; i < seeds.size; ++i) {
                Interval ii = seeds[i];
                Interval affected = intersect(ii, source);
                if (affected.empty()) continue;
                
                array_push_back(&seeds_new, {affected.start + diff, affected.end + diff});
                seeds[i] = seeds.back();
                --seeds.size;
                --i;
                subtract(ii, source, &seeds);
            }
        }
        
        array_append(&seeds, seeds_new);
        seeds_new.size = 0;
        
        format_print("B ");
        for (auto q: seeds) {
            format_print("[%d,%d), ", q.start, q.end);
        }
        format_print("\n");
        
        array_sort_heap(&seeds, [](Interval i) {
                            return (u64)(i.start << 32 | i.end);
                        });
        assert(seeds.size);
        
        format_print("c ");
        for (auto q: seeds) {
            format_print("[%d,%d), ", q.start, q.end);
        }
        format_print("\n");
        
        s64 j = 0;
        for (s64 i = 1; i < seeds.size; ++i) {
            auto a = seeds[j];
            auto b = seeds[i];
            if (b.start < a.end) {
                // overlap; join intervals
                if (seeds[j].end < b.end) seeds[j].end = b.end;
            } else {
                ++j;
                seeds[j] = seeds[i];
            }
        }
        seeds.size = j+1;
        
        format_print("C ");
        for (auto q: seeds) {
            format_print("[%d,%d), ", q.start, q.end);
        }
        format_print("\n");
        
    }
    
    s64 min = seeds[0].start;
    for (Interval i: seeds) {
        if (min > i.start) min = i.start;
    }
    format_print("%d\n", min);
}