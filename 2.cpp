#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"

int main() {
    auto arr = array_load_from_file("input/2"_arr);
    s64 number = 0;
    u8 color = -1;
    s64 game_id = -1;
    bool possible = true;
    s64 sum = 0;
    
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if ('0' <= c and c <= '9') {
            number = 10 * number + (c - '0');
        } else if (c == ':') {
            game_id = number;
            number = 0;
        } else if (c == ' ') {
            color = arr[i+1] % 3;
        } else if (c == ',' or c == ';' or c == '\n') {
            if (number > 12+color) {
                possible = false;
            }
            number = 0;
            color = -1;
        }
        if (c == '\n') {
            assert(game_id != -1);
            if (possible) sum += game_id;
            possible = true;
            game_id = -1;
        }
    }
    format_print("%d\n", sum);
}