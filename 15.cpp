#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/15"_arr);
    
    u8 hash = 0;
    s64 sum = 0;
    for (u8 c: arr) {
        if (c == ',' or c == '\n') {
            sum += hash;
            hash = 0;
        } else {
            hash = (hash + c) * 17;
        }
    }
    
    format_print("%d\n", sum);
}
