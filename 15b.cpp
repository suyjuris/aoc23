#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/15"_arr);
    
    struct Lens {
        Array_t<u8> name;
        u8 focus;
    };
    Array_t<Array_dyn<Lens>> boxes = array_create<Array_dyn<Lens>>(256);
    s64 last = 0;
    for (s64 i = 0; i < arr.size; ++i) {
        u8 c = arr[i];
        
        Array_t<u8> name;
        u8 hash = 0;
        if (c == '=' or c == '-') {
            name = array_subarray(arr, last, i);
            for (u8 c: name) hash = (hash + c) * 17;
        }
        auto* box = &boxes[hash];
        
        if (c == '=') {
            last = i+3;
            u8 focus = arr[i+1] - '0';
            i = last-1;
            //format_print("After \"%a=%d\":\n", name, focus);
            
            bool flag = false;
            for (s64 j = 0; j < box->size; ++j) {
                if (array_equal((*box)[j].name, name)) {
                    (*box)[j].focus = focus;
                    flag = true;
                    break;
                }
            }
            if (not flag) {
                array_push_back(box, {name, focus});
            }
            
        } else if (c == '-') {
            last = i+2;
            i = last-1;
            //format_print("After \"%a-\":\n", name);
            
            for (s64 j = 0; j < box->size; ++j) {
                if (array_equal((*box)[j].name, name)) {
                    for (s64 k = j; k+1 < box->size; ++k) {
                        (*box)[k] = (*box)[k+1];
                    }
                    --box->size;
                    break;
                }
            }
        } else continue;
        
        /*for (s64 i = 0; i < boxes.size; ++i) {
            auto box = boxes[i];
            if (box.size == 0) continue;
            format_print("Box %d:", i);
            for (s64 j = 0; j < box.size; ++j) {
                format_print(" [%a %d]", box[j].name, box[j].focus);
            }
            format_print("\n");
        }
        format_print("\n");*/
    }
    
    s64 sum = 0;
    for (s64 i = 0; i < boxes.size; ++i) {
        auto box = boxes[i];
        for (s64 j = 0; j < box.size; ++j) {
            sum += (i+1) * (j+1) * box[j].focus;
        }
    }
    format_print("%d\n", sum);
}
