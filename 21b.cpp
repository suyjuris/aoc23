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
        s64 steps = 0;
    };
    auto frontier = heap_init<Pos>([](Pos p) { return -p.steps; });
    
    s64 w = width;
    
    s64 even = 0;
    s64 odd = 0;
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) == '#') continue;
            even += ~(x+y) & 1;
            odd += (x+y) & 1;
        }
    }
    
    
    Pos start;
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) != 'S') continue;
            start = {x, y, 0};
            break;
        }
    }
    
    if ((start.x+start.y)&1) simple_swap(&even, &odd);
    
    auto dist = [&](Pos p, Pos q) {
        return abs(p.y - q.y) + abs(p.x - q.x);
    };
    
    Array_dyn<Pos> competitive;
    array_memset(&visited);
    heap_push(&frontier, start);
    while (frontier.arr.size) {
        Pos p = heap_pop(&frontier);
        if (get(p.x, p.y) == '#') continue;
        if (bitset_get(visited, p.x + p.y*width)) continue;
        
        bitset_set(&visited, p.x + p.y*width, true);
        if (p.x == 0 or p.x == w-1 or p.y == 0 or p.y == w-1) {
            bool flag = false;
            for (Pos j: competitive) {
                if (p.steps - j.steps >= dist(p, j)) {
                    flag = true;
                    break;
                }
            }
            if (not flag) array_push_back(&competitive, p);
        }
        
        heap_push(&frontier, {p.x+1, p.y, p.steps+1});
        heap_push(&frontier, {p.x, p.y+1, p.steps+1});
        heap_push(&frontier, {p.x-1, p.y, p.steps+1});
        heap_push(&frontier, {p.x, p.y-1, p.steps+1});
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
    
    //s64 total = 5000;
    s64 total = 26501365;
    s64 maxd = (total - competitive[0].steps-1) / w + 3;
    s64 sum = 0;
    //format_print("%d %d\n", even, odd);
    
    Array_dyn<s64> temp;
    
    Hashmap<s64> cache;
    
    s64 lastp = 0;
    for (s64 cy = -maxd; cy <= maxd; ++cy) {
        s64 cx0 = -maxd + abs(cy);
        s64 cx3 = maxd - abs(cy);
        
        temp.size = 0;
        array_push_back(&temp, cx0);
        for (s64 i: {cx0+1, cx0+2, cx3-2, cx3-1, cx3}) {
            if (temp.back() < i and i <= cx3)
                array_push_back(&temp, i);
        }
        
        //format_print("%d:%d, %d\n", cx0, cx3, cy);
        
        s64 len = cx3+1-cx0 - temp.size;
        if (len > 0) {
            s64 f = cx0+2+cy;
            sum += (len/2 + (f&len&1)) * even;
            sum += (len/2 + (~f&len&1)) * odd;
            //format_print("  %d even, %d odd\n", (len/2 + (f&len&1)), (len/2 + (~f&len&1)));
            //for (s64 i = cx0+3; i < cx3-2; ++i) {
            //format_print("%d,%d -> %d !\n", i, cy, (i&1) ? odd : even);
            //}
        }
        
        for (s64 cx: temp) {
            //format_print("  [%d]  ", cx);
            u64 h = 0;
            frontier.arr.size = 0;
            for (s64 i = 0; i < 4; ++i) {
                Pos pi = cpos(i);
                pi.x += cx*w;
                pi.y += cy*w;
                
                s64 d = total+1;
                Pos pj;
                for (Pos p: competitive) {
                    s64 dd = p.steps + dist(p, pi);
                    if (d > dd) {
                        d = dd;
                        pj = p;
                    }
                }
                h = hash_u64_pair(h, d);
                
                if (d > total) continue;
                heap_push(&frontier, cpos(i, d));
                //format_print("%d_%d(%d,%d) ", i, d, cpos(i).x, cpos(i).y);
            }
            
            if (s64* ptr = hashmap_getptr(&cache, h)) {
                //format_print("-> %d c\n", *ptr);
                //format_print("%d,%d -> %d !\n", cx, cy, *ptr);
                sum += *ptr;
                continue;
            }
            
            s64 count = 0;
            array_memset(&visited);
            while (frontier.arr.size) {
                Pos p = heap_pop(&frontier);
                if (get(p.x, p.y) == '#') continue;
                if (bitset_get(visited, p.x + p.y*width)) continue;
                if (p.steps > total) continue;
                
                bitset_set(&visited, p.x + p.y*width, true);
                sum += ~p.steps&1;
                count += ~p.steps&1;
                
                heap_push(&frontier, {p.x+1, p.y, p.steps+1});
                heap_push(&frontier, {p.x, p.y+1, p.steps+1});
                heap_push(&frontier, {p.x-1, p.y, p.steps+1});
                heap_push(&frontier, {p.x, p.y-1, p.steps+1});
            }
            
            //format_print("-> %d\n", count);
            //format_print("%d,%d -> %d !\n", cx, cy, count);
            
            hashmap_set(&cache, h, count);
        }
    }
    
    format_print("%d\n", sum);
}