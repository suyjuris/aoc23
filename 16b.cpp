#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/16"_arr);
    
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
    
    Array_t<u64> beams = array_create<u64>((width*height*4 + 63) / 64);
    
    struct Dpos {
        s64 x, y;
        u8 d;
    };
    Array_dyn<Dpos> stack;
    
    auto move = [](Dpos dp, u8 d) -> Dpos {
        switch (d) {
            case 0: return {dp.x+1, dp.y, d};
            case 1: return {dp.x, dp.y-1, d};
            case 2: return {dp.x-1, dp.y, d};
            case 3: return {dp.x, dp.y+1, d};
            default: assert(false);
        }
    };
    
    auto energize = [&](Dpos start) {
        array_memset(&beams);
        array_push_back(&stack, start);
        while (stack.size) {
            auto dp = stack.back();
            --stack.size;
            
            if (not (0 <= dp.x and dp.x < width and 0 <= dp.y and dp.y < height)) continue;
            
            {
                s64 index = (dp.y * width + dp.x) * 4 + dp.d;
                if (bitset_get(beams, index)) continue;
                bitset_set(&beams, index, true);
            }
            
            u8 c = arr[dp.y * (width+1) + dp.x];
            if (c == '.') {
                array_push_back(&stack, move(dp, dp.d));
            } else if (c == '/') {
                array_push_back(&stack, move(dp, dp.d ^ 1));
            } else if (c == '\\') {
                array_push_back(&stack, move(dp, dp.d ^ 3));
            } else if (c == '|') {
                if (dp.d&1) {
                    array_push_back(&stack, move(dp, dp.d));
                } else {
                    array_push_back(&stack, move(dp, dp.d ^ 1));
                    array_push_back(&stack, move(dp, dp.d ^ 3));
                }
            } else if (c == '-') {
                if (~dp.d&1) {
                    array_push_back(&stack, move(dp, dp.d));
                } else {
                    array_push_back(&stack, move(dp, dp.d ^ 1));
                    array_push_back(&stack, move(dp, dp.d ^ 3));
                }
            }
        }
        
        s64 sum = 0;
        for (s64 y = 0; y < height; ++y) {
            for (s64 x = 0; x < width; ++x) {
                s64 energized = false;
                for (u8 d = 0; d < 4; ++d) {
                    energized |= bitset_get(beams, (y * width + x) * 4 + d);
                }
                sum += energized;
            }
        }
        return sum;
    };
    
    s64 result = 0;
    for (s64 y = 0; y < height; ++y) {
        result = max(result, energize({0, y, 0}));
        result = max(result, energize({width-1, y, 2}));
    }
    for (s64 x = 0; x < width; ++x) {
        result = max(result, energize({x, 0, 3}));
        result = max(result, energize({x, height-1, 1}));
    }
    
    format_print("%d\n", result);
}
