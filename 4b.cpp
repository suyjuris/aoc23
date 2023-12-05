#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/4"_arr);
    
    s64 number = 0;
    s64 sum = 0;
    Array_dyn<u8> winning;
    s64 state = 0;
    s64 winning_count = 0;
    
    Array_t<s64> copies = array_create<s64>(1024*1024); // Stupid? Yes. But works, and does not even waste memory (due to linux COW semantics)
    s64 lineno = 0;
    
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if ('0' <= c and c <= '9') {
            number = 10*number + (c - '0');
        } else if ((c == ' ' or c == '\n') and number) {
            if (state == 0) {
                array_push_back(&winning, number);
            } else {
                bool is_winning = false;
                for (u8 j: winning) is_winning |= j == number;
                winning_count += is_winning;
            }
            number = 0;
        } else if (c == '|') {
            state = 1;
        } else if (c == ':') {
            number = 0;
        }
        
        if (c == '\n') {
            s64 multiplier = copies[lineno] + 1;
            sum += multiplier;
            for (s64 j = 0; j < winning_count; ++j) {
                copies[lineno+j+1] += multiplier;
            }
            winning.size = 0;
            state = 0;
            winning_count = 0;
            ++lineno;
        }
    }
    
    format_print("%d\n", sum);
}