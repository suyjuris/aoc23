#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/21"_arr);
    
    s64 width = -1;
    for (s64 i = 0; i < arr.size; ++i) {
        if (arr[i] == '\n') {
            width = i;
            break;
        }
    }
    assert(width != -1);
    assert(arr.size % (width + 1) == 0);
    s64 height = arr.size / (width + 1);
    
    auto get = [&arr, width, height](s64 x, s64 y) {
        if (0 <= x and x < width and 0 <= y and y < height) {
            return arr[x + y*(width+1)];
        } else {
            return (u8)'#';
        }
    };
    
    Array_t<u64> visited = array_create<u64>((width * height + 63) / 64);
    
    struct Pos {
        s64 x, y;
        s64 steps;
    };
    auto frontier = heap_init<Pos>([](Pos p) { return -p.steps; });
    
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) != 'S') continue;
            heap_push(&frontier, {x, y, 0});
            break;
        }
    }
    
    s64 sum = 0;
    while (frontier.arr.size) {
        Pos p = heap_pop(&frontier);
        if (get(p.x, p.y) == '#') continue;
        if (bitset_get(visited, p.x + p.y*width)) continue;
        if (p.steps > 64) continue;
        
        bitset_set(&visited, p.x + p.y*width, true);
        sum += ~p.steps&1;
        
        heap_push(&frontier, {p.x+1, p.y, p.steps+1});
        heap_push(&frontier, {p.x, p.y+1, p.steps+1});
        heap_push(&frontier, {p.x-1, p.y, p.steps+1});
        heap_push(&frontier, {p.x, p.y-1, p.steps+1});
    }
    format_print("%d\n", sum);
}