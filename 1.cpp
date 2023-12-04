#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"

int main() {
    auto arr = array_load_from_file("input/1"_arr);
    s64 sum = 0;
    s64 first = -1, last = -1;
    for (u8 c: arr) {
        if ('0' <= c and c <= '9') {
            s64 val = c - '0';
            if (first == -1) first = val;
            last = val;
        } else if (c == '\n') {
            sum += 10*first + last;
            first = -1;
            last = -1;
        }
    }
    format_print("%d\n", sum);
}