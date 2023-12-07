#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

struct Hand {
    u64 val = 0;
    s64 bid = 0;
};

int main() {
    auto arr = array_load_from_file("input/7"_arr);
    Array_t<s64> counts = array_create<s64>(13);
    Array_dyn<s64> counts_sorted;
    Array_dyn<Hand> hands;
    
    for (s64 index = 0; index < arr.size; ++index) {
        Array_t<u8> cards = array_subarray(arr, index, index+5);
        index += 6;
        
        array_memset(&counts);
        for (u8& i: cards) {
            switch (i) {
                case 'A': i = 0; break;
                case 'K': i = 1; break;
                case 'Q': i = 2; break;
                case 'J': i = 3; break;
                case 'T': i = 4; break;
                case '9': i = 5; break;
                case '8': i = 6; break;
                case '7': i = 7; break;
                case '6': i = 8; break;
                case '5': i = 9; break;
                case '4': i = 10; break;
                case '3': i = 11; break;
                case '2': i = 12; break;
                default: assert(false);
            }
            ++counts[i];
        }
        
        counts_sorted.size = 0;
        for (s64 i: counts) {
            if (i) array_push_back(&counts_sorted, i);
        }
        array_sort_insertion(&counts_sorted, [](s64 x) { return -x; });
        
        Hand hand;
        u8 type;
        if (counts_sorted[0] == 5) {
            type = 0;
        } else if (counts_sorted[0] == 4) {
            type = 1;
        } else if (counts_sorted[0] == 3 and counts_sorted[1] == 2) {
            type = 2;
        } else if (counts_sorted[0] == 3) {
            type = 3;
        } else if (counts_sorted[0] == 2 and counts_sorted[1] == 2) {
            type = 4;
        } else if (counts_sorted[0] == 2) {
            type = 5;
        } else {
            type = 6;
        }
        
        hand.val = 0ull
            | (u64)type << 28
            | (u64)cards[0] << 24
            | (u64)cards[1] << 20
            | (u64)cards[2] << 16
            | (u64)cards[3] << 12
            | (u64)cards[4] <<  8
            ;
        
        hand.bid = 0;
        while ('0' <= arr[index] and arr[index] <= '9') {
            hand.bid = 10*hand.bid + (arr[index] - '0');
            ++index;
        }
        
        array_push_back(&hands, hand);
    }
    
    array_sort_heap(&hands, [](Hand h) { return h.val; });
    s64 sum = 0;
    for (s64 i = 0; i < hands.size; ++i) {
        sum += hands[i].bid * (hands.size - i);
    }
    
    format_print("%d\n", sum);
}