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
    
    Pos start;
    for (s64 y = 0; y < height; ++y) {
        for (s64 x = 0; x < width; ++x) {
            if (get(x, y) != 'S') continue;
            start = {x, y, 0};
            break;
        }
    }
    
    if ((start.x+start.y)&1) simple_swap(&even, &odd);
    
    auto dist = [&](Pos p, Pos q) -> s64 {
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
        even += ~p.steps&1;
        odd += p.steps&1;
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
    auto mpos = [&](u8 c, s64 s=0) -> Pos {
        s64 k = (w-1)/2;
        switch (c) {
            case 0: return {k, 0, s};
            case 1: return {k, w-1, s};
            case 2: return {0, k, s};
            case 3: return {w-1, k, s};
            default: assert(false); return {-1, -1};
        }
    };
    
    //s64 total = 337;
    for (s64 total: {1001, 26501365})
        //for (s64 total: {50, 100, 500, 1000, 5000})
    /*for (s64 total: {67,85,103,121,139,157,175,193,211,229,247,265,283,301,319,337,355,373,
             391,409,427,445,463,481,499,517,535,553,571,589,607,625,643,661,679,697,715,733,751,
                      769,787,805,823,841,859,877,895,913,931,949,967,985})*/
    {
        //s64 total = 26501365;
        s64 maxd = (total - competitive[0].steps-1) / w + 2;
        s64 sum = 0;
        //format_print("%d %d\n", even, odd);
        
        Array_dyn<s64> temp;
        
        Hashmap<s64> cache;
        
        s64 lastp = 0;
        for (s64 cy = -maxd; cy <= maxd; ++cy) {
            s64 cx0 = -maxd + abs(cy);
            s64 cx3 =  maxd - abs(cy);
            s64 cx1 = cx0, cx2 = cx0;
            bool flag1 = false;
            bool flag2 = false;
            for (s64 iter = 0; iter < cx3-cx0+1 and not (flag1 and flag2); ++iter) {
                if (iter%2 ? flag2 : flag1) continue;
                s64 cx = iter%2 ? cx3 - iter/2 : cx0 + iter/2;
                
                u64 h = 0;
                frontier.arr.size = 0;
                if ((cx == 0 or cy == 0) and w > 100) {
                    h = hash_u64_pair(h, 987981749812793871);
                    for (s64 i = 0; i < 4; ++i) {
                        Pos pi = mpos(i);
                        pi.x += cx*w;
                        pi.y += cy*w;
                        
                        s64 d = min(total+1, dist(pi, start));
                        h = hash_u64_pair(h, d);
                        
                        if (d > total) continue;
                        heap_push(&frontier, mpos(i, d));
                    }
                } else {
                    for (s64 i = 0; i < 4; ++i) {
                        Pos pi = cpos(i);
                        pi.x += cx*w;
                        pi.y += cy*w;
                        
                        s64 d = total+1;
                        for (Pos p: competitive) {
                            s64 dd = p.steps + dist(p, pi);
                            if (d > dd) d = dd;
                        }
                        h = hash_u64_pair(h, d);
                        
                        if (d > total) continue;
                        heap_push(&frontier, cpos(i, d));
                    }
                }
                
                s64 count = 0;
                if (s64* ptr = hashmap_getptr(&cache, h)) {
                    count = *ptr;
                } else {
                    array_memset(&visited);
                    while (frontier.arr.size) {
                        Pos p = heap_pop(&frontier);
                        if (get(p.x, p.y) == '#') continue;
                        if (bitset_get(visited, p.x + p.y*width)) continue;
                        if (p.steps > total) continue;
                        
                        bitset_set(&visited, p.x + p.y*width, true);
                        count += (~total^p.steps)&1;
                        
                        heap_push(&frontier, {p.x+1, p.y, p.steps+1});
                        heap_push(&frontier, {p.x, p.y+1, p.steps+1});
                        heap_push(&frontier, {p.x-1, p.y, p.steps+1});
                        heap_push(&frontier, {p.x, p.y-1, p.steps+1});
                    }
                    
                    hashmap_set(&cache, h, count);
                }
                sum += count;
                
                if (iter%2) cx2 = cx;
                else        cx1 = cx;
                if (count == (((cx+cy)^total)&1 ? odd : even)) {
                    if (iter%2) flag2 = true;
                    else        flag1 = true;
                }
                
                //format_print("%d %d %d %d%d -> %d\n", cx, cy, iter, flag1, flag2, count);
                //format_print("%d,%d -> %d !\n", cx, cy, count);
            }
            
            s64 len = cx2-1 - cx1;
            if (len > 0) {
                s64 f = (cx1+1+cy) ^ total;
                sum += (len/2 + (~f&len&1)) * even;
                sum += (len/2 + (f&len&1)) * odd;
                //format_print("%d even, %d odd\n", (len/2 + (~f&len&1)), (len/2 + (f&len&1)));
            }
        }
        format_print("%d %d\n", total, sum);
    }
}