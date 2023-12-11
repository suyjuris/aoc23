#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

s64 abss(s64 x) {
    return x >= 0 ? x : -x;
}

int main() {
    auto arr = array_load_from_file("input/11"_arr);
    
    struct Pos {
        s64 x, y;
        s64 ox, oy;
    };
    Array_dyn<Pos> galaxies;
    
    {
        s64 x = 0, y = 0;
        for (u8 c: arr) {
            if (c == '#') {
                array_push_back(&galaxies, {x, y, x, y});
            } else if (c == '\n') {
                x = 0;
                y += 1;
                continue;
            }
            x += 1;
        }
    }
    
    {
        array_sort_heap(&galaxies, [](Pos p) { return p.x; });
        
        s64 expansion = 0;
        s64 last_x = 0;
        for (Pos& p: galaxies) {
            if (p.x > last_x) {
                expansion += p.x - last_x - 1;
                last_x = p.x;
            }
            p.x += expansion;
        }
    }
    
    {
        array_sort_heap(&galaxies, [](Pos p) { return p.y; });
        s64 expansion = 0;
        s64 last_y = 0;
        for (Pos& p: galaxies) {
            if (p.y > last_y) {
                expansion += p.y - last_y - 1;
                last_y = p.y;
            }
            p.y += expansion;
        }
    }
    
    s64 sum = 0;
    for (s64 i = 0; i+1 < galaxies.size; ++i) {
        for (s64 j = i+1; j < galaxies.size; ++j) {
            Pos pi = galaxies[i];
            Pos pj = galaxies[j];
            sum += abss(pi.x - pj.x) + abss(pi.y - pj.y);
        }
    }
    
    format_print("%d\n", sum);
}