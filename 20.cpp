#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/20"_arr);
    
    struct Output {
        s64 to;
        s64 index;
    };
    Array_dyn<Output> outputs_data;
    
    struct Gate {
        u8 type;
        Offset<Output> outputs;
        u64 state = 0;
        s64 num_inputs = 0;
    };
    Array_dyn<Gate> gates;
    
    Hashmap<s64> id_to_gate;
    id_to_gate.empty = -1;
    
    s64 index = 0;
    
    auto read_id = [&]() {
        u64 id = 0;
        s64 i = 0;
        while (('a' <= arr[index] and arr[index] <= 'z') or ('A' <= arr[index] and arr[index] <= 'Z')) {
            assert(i < 8);
            id |= (u64)arr[index] << (i*8);
            ++index;
            ++i;
        }
        return id;
    };
    
    auto read_char = [&](u8 c) {
        if (arr[index] != c) {
            format_print("%a %a\n", Array_t<u8> {&c, 1}, array_subarray(arr, index, min(index+20, arr.size)));
        }
        assert(arr[index] == c);
        ++index;
    };
    
    array_push_back(&gates, {});
    hashmap_set(&id_to_gate, 0, 0);
    
    while (index < arr.size) {
        u8 c = arr[index];
        
        u64 id;
        if (c == 'b') {
            while (arr[index] != ' ') ++index;
            id = 0;
        } else {
            ++index;
            id = read_id();
        }
        
        s64 i = *hashmap_getcreate(&id_to_gate, id, gates.size);
        if (i == gates.size) {
            array_push_back(&gates, {});
        }
        gates[i].type = c;
        
        read_char(' ');
        read_char('-');
        read_char('>');
        read_char(' ');
        
        s64 output_i = outputs_data.size;
        
        while(arr[index] != '\n') {
            u64 to = read_id();
            
            s64 j = *hashmap_getcreate(&id_to_gate, to, gates.size);
            if (j == gates.size) {
                array_push_back(&gates, {});
            }
            array_push_back(&outputs_data, {j, gates[j].num_inputs++});
            if (arr[index] == ',') index += 2;
        }
        ++index;
        
        gates[i].outputs = array_makeoffset(outputs_data, output_i);
    }
    
    s64 sum_low = 0;
    s64 sum_high = 0;
    
    struct Pulse {
        s64 dest;
        s64 index;
        bool high;
        u64 ts;
    };
    u64 time = 0;
    auto stack = heap_init<Pulse>([](Pulse p) { return -p.ts; });
    for (s64 i = 0; i < 1000; ++i) {
        heap_push(&stack, {0, 0, false, time++});
        while (stack.arr.size) {
            auto p = heap_pop(&stack);
            //format_print("%d.%d %d\n", p.dest, p.index, p.high);
            
            sum_low += not p.high;
            sum_high += p.high;
            
            Gate* g = &gates[p.dest];
            bool send_high;
            if (g->type == 'b') {
                send_high = p.high;
            } else if (g->type == '%') {
                if (p.high) continue;
                g->state ^= 1;
                send_high = g->state;
            } else if (g->type == '&') {
                Array_t<u64> arr {&g->state, 1};
                bitset_set(&arr, p.index, p.high);
                u64 mask = (1 << g->num_inputs) - 1;
                send_high = (g->state & mask) != mask;
            } else continue;
            
            for (auto i: array_suboffset(outputs_data, g->outputs)) {
                heap_push(&stack, {i.to, i.index, send_high, time++});
            }
        }
    }
    
    format_print("%d\n", sum_high * sum_low);
}