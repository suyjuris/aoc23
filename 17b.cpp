#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/17"_arr);
    
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
    
    for (u8& c: arr) {
        c -= '0';
    }
    
    struct Node {
        s64 x, y;
        u8 d;
        u8 len;
        u64 key() {
            return x << 48 | y << 24 | d << 8 | len;
        }
    };
    
    
    auto move = [](Node dp, u8 d) -> Node {
        u8 newlen = dp.d == d ? dp.len + 1 : 0;
        switch (d) {
            case 0: return {dp.x+1, dp.y, d, newlen};
            case 1: return {dp.x, dp.y-1, d, newlen};
            case 2: return {dp.x-1, dp.y, d, newlen};
            case 3: return {dp.x, dp.y+1, d, newlen};
            default: assert(false);
        }
    };
    
    Hashmap<s64> cache;
    cache.empty = -1;
    
    struct Entry {
        Node node;
        s64 dist;
    };
    auto stack = heap_init<Entry>([](Entry e) { return -e.dist; });
    array_push_back(&stack.arr, {{0, 0, 0, 0}, 0});
    hashmap_set(&cache, stack.arr[0].node.key(), 0);
    
    s64 sum = -1;
    while (stack.arr.size) {
        auto entry = heap_pop(&stack);
        if (entry.dist > hashmap_get(&cache, entry.node.key())) continue;
        if (entry.node.x == width-1 and entry.node.y == height-1) {
            sum = entry.dist;
            break;
        }
        
        for (u8 d = 0; d < 4; ++d) {
            if (d == (entry.node.d ^ 2)) continue;
            if (entry.node.len < 3 and d != entry.node.d) continue;
            Node dp = move(entry.node, d);
            if (dp.len >= 10) continue;
            if (not (0 <= dp.x and dp.x < width and 0 <= dp.y and dp.y < height)) continue;
            Entry e = {dp, entry.dist + arr[dp.x + dp.y * (width+1)]};
            s64* ptr = hashmap_getcreate(&cache, dp.key(), e.dist+1);
            if (*ptr > e.dist) {
                *ptr = e.dist;
                heap_push(&stack, e);
            }
        }
    }
    format_print("%d\n", sum);
}
