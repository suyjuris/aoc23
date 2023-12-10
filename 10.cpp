#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"


int main() {
    auto arr = array_load_from_file("input/10"_arr);
    
    s64 width = 0;
    while (arr[width] != '\n') ++width;
    ++width;
    
    assert(arr.size % width == 0);
    s64 height = arr.size / width;
    
    auto get = [&arr, width, height](s64 x, s64 y) {
        if (0 <= x and x+1 < width and 0 <= y and y < height) {
            return arr[x + y*width];
        } else {
            return (u8)'.';
        }
    };
    
    Array_t<u64> visited = array_create<u64>((width * height + 63) / 64);
    
    struct Pos {
        s64 x, y;
        u8 from;
        s64 dist;
    };
    Array_dyn<Pos> stack;
    
    for (s64 i = 0; i < arr.size; ++i) {
        if (arr[i] == 'S') {
            s64 x = i%width;
            s64 y = i/width;
            array_push_back(&stack, {x+1, y, (u8)2, 1});
            array_push_back(&stack, {x-1, y, (u8)0, 1});
            array_push_back(&stack, {x, y-1, (u8)3, 1});
            array_push_back(&stack, {x, y+1, (u8)1, 1});
        }
    }
    
    s64 final_dist = -1;
    
    while (stack.size) {
        array_sort_insertion(&stack, [](Pos i) { return -i.dist; });
        
        for (Pos i: stack) {
            u8 c = get(i.x, i.y);
            //format_print("{%d,%d %d %d %a} ", i.x, i.y, i.from, i.dist, Array_t<u8> {&c, 1});
        }
        //format_print("\n");
        
        Pos p = stack.back();
        --stack.size;
        
        u8 c = get(p.x, p.y);
        if (c == '.') continue;
        
        u8 a, b;
        switch (c) {
            case '|': a = 1; b = 3; break;
            case '-': a = 0; b = 2; break;
            case 'L': a = 0; b = 1; break;
            case 'J': a = 1; b = 2; break;
            case '7': a = 2; b = 3; break;
            case 'F': a = 3; b = 0; break;
            default: assert(false);
        }
        
        if (a == p.from) {
            // nothing
        } else if (b == p.from) {
            simple_swap(&a, &b);
        } else {
            continue;
        }
        
        switch (b) {
            case 0: p.x += 1; break;
            case 1: p.y -= 1; break;
            case 2: p.x -= 1; break;
            case 3: p.y += 1; break;
            default: assert(false);
        }
        p.dist += 1;
        p.from = b ^ 2;
        
        if (bitset_get(visited, p.x+p.y*width)) {
            final_dist = p.dist;
            break;
        }
        bitset_set(&visited, p.x+p.y*width, true);
        
        array_push_back(&stack, p);
    }
    
    format_print("%d\n", final_dist);
}