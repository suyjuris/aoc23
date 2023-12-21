#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/21hint"_arr);
    
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
        s64 steps = 0;
    };
    auto frontier = heap_init<Pos>([](Pos p) { return -p.steps; });
    
    s64 w = width;
    
    s64 even = 0;
    s64 odd = 0;
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) == '#') continue;
            even += (x+y) & 1;
            odd += ~(x+y) & 1;
        }
    }
    
    Array_dyn<s64> corners;
    array_append(&corners, {10, 10, 10, 14});
    //array_append(&corners, {130, 130, 130, 130});
    
    Pos start;
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) != 'S') continue;
            start = {x, y, 0};
            break;
        }
    }
    
    auto cpos = [&](u8 c, s64 s=0) -> Pos {
        switch (c) {
            case 0: return {0, 0,s};
            case 1: return {w-1, 0,s};
            case 2: return {0, w-1,s};
            case 3: return {w-1, w-1,s};
            default: assert(false); return {-1, -1};
        }
    };
    
    s64 total = 50;
    s64 maxd = (total - corners[0]+1) / w;
    s64 sum = 0;
    for (s64 cy = -maxd; cy <= maxd; ++cy) {
        s64 cx0 = -maxd + abs(cy);
        s64 cx1 = maxd - abs(cy);
        
        format_print("%d:%d %d: %d\n", cx0, cx1, cy, sum);
        if (cx0 != cx1) {
            sum += ((cx1-cx0-2)/2 + (~cx0&1)) * even;
            sum += ((cx1-cx0-2)/2 + (cx0&1)) * odd;
        }
        
        bool flag = false;
        for (s64 cx: {cx0, cx1}) {
            if (cx0 == cx1 and flag) break;
            flag = true;
            Array_t<s64> ccs {0, 0, 0, 0};
            
            for (s64 i = 0; i < 4; ++i) {
                s64 j = i;
                if (cx < 0) j &= ~1;
                if (cx > 0) j |= 1;
                if (cy < 0) j &= ~2;
                if (cy > 0) j |= 2;
                ccs[i] = corners[j];
                
                Pos pj = cpos(j);
                Pos pi = cpos(i);
                pi.x += cx*w;
                pi.y += cy*w;
                
                ccs[i] += abs(pj.x - pi.x);
                ccs[i] += abs(pj.y - pi.y);
                
                heap_push(&frontier, cpos(i, ccs[i]));
            }
            format_print("%d %d: %d\n", cx, cy, sum);
            
            array_memset(&visited);
            while (frontier.arr.size) {
                Pos p = heap_pop(&frontier);
                if (get(p.x, p.y) == '#') continue;
                if (bitset_get(visited, p.x + p.y*width)) continue;
                if (p.steps > total) continue;
                
                bitset_set(&visited, p.x + p.y*width, true);
                sum += ~p.steps&1;
                
                heap_push(&frontier, {p.x+1, p.y, p.steps+1});
                heap_push(&frontier, {p.x, p.y+1, p.steps+1});
                heap_push(&frontier, {p.x-1, p.y, p.steps+1});
                heap_push(&frontier, {p.x, p.y-1, p.steps+1});
            }
            format_print("%d %d: %d\n", cx, cy, sum);
        }
    }
    
    format_print("%d\n", sum);
}