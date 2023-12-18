#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/18"_arr);
    
    struct Pos {
        s64 x = 0, y = 0;
    };
    
    auto move = [](Pos dp, u8 d, s64 a) -> Pos {
        switch (d) {
            case 0: return {dp.x+a, dp.y};
            case 1: return {dp.x, dp.y-a};
            case 2: return {dp.x-a, dp.y};
            case 3: return {dp.x, dp.y+a};
            default: assert(false);
        }
    };
    
    struct Seg {
        Pos a, b;
        u8 d;
        s64 len;
    };
    
    Array_dyn<Seg> segs;
    
    Pos p;
    Pos maxp, minp;
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        if (c == '\n') break;
        
        // This is not the correct parsing routine, but I misplaced the other one
        while (arr[i] != '#') ++i;
        ++i;
        s64 a = 0;
        for (s64 j = 0; j < 5; ++j) {
            u8 c = arr[i];
            if ('0' <= arr[i] and arr[i] <= '9') {
                a = 16*a + (arr[i] - '0');
            } else if ('a' <= arr[i] and arr[i] <= 'f') {
                a = 16*a + (arr[i] - 'a' + 10);
            } else assert(false);
            
            ++i;
        }
        
        u8 d = arr[i] - '0';
        d ^= 1;
        
        while (arr[i] != '\n') ++i;
        
        Seg s;
        s.a = p;
        p = move(p, d, a);
        s.b = p;
        s.d = d;
        s.len = a;
        
        array_push_back(&segs, s);
        
        maxp.x = max(maxp.x, p.x);
        maxp.y = max(maxp.y, p.y);
        minp.x = min(minp.x, p.x);
        minp.y = min(minp.y, p.y);
    }
    assert(p.x == 0 and p.y == 0);
    
    for (Seg& i: segs) {
        i.a.x -= minp.x;
        i.a.y -= minp.y;
        i.b.x -= minp.x;
        i.b.y -= minp.y;
    }
    
    for (s64 i = 0; i < segs.size; ++i) {
        if (segs[i].d & 1) continue;
        segs[i] = segs.back();
        --i;
        --segs.size;
    }
    for (Seg& i: segs) {
        if (i.d == 1) {
            simple_swap(&i.a, &i.b);
            i.d = 3;
        }
    }
    
    Pos size = maxp;
    size.x -= minp.x;
    size.y -= minp.y;
    ++size.x; ++size.y;
    
    s64 sum = 0;
    array_sort_heap(&segs, [](Seg s) { return s.a.y; });
    
    for (s64 y = 0; y < size.y; ++y) {
        bool inside0 = false;
        bool inside1 = false;
        for (s64 x = 0; x < size.x; ++x) {
            bool flip0 = false;
            bool flip1 = false;
            for (Seg i: segs) {
                if (i.a.x != x) continue;
                if (not (i.a.y <= y and y <= i.b.y)) continue;
                flip0 |= i.a.y < y;
                flip1 |= i.b.y > y;
            }
            sum += inside0 or inside1 or flip0 or flip1;
            inside0 ^= flip0;
            inside1 ^= flip1;
            
        }
    }
    format_print("%d\n", sum);
}