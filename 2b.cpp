#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"

int main() {
    auto arr = array_load_from_file("input/2"_arr);
    s64 number = 0;
    u8 color = -1;
    auto game_max = array_create<s64>(3);
    s64 sum = 0;
    
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if ('0' <= c and c <= '9') {
            number = 10 * number + (c - '0');
        } else if (c == ':') {
            number = 0;
        } else if (c == ' ') {
            color = arr[i+1] % 3;
        } else if (c == ',' or c == ';' or c == '\n') {
            if (game_max[color] < number) {
                game_max[color] = number;
            }
            number = 0;
            color = -1;
        }
        if (c == '\n') {
            sum += game_max[0] * game_max[1] * game_max[2];
            array_memset(&game_max);
        }
    }
    format_print("%d\n", sum);
}