#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

int main() {
    auto arr = array_load_from_file("input/19"_arr);
    
    struct Step {
        u64 property;
        s8 cmp;
        s64 value;
        u64 next_step;
    };
    Array_dyn<Step> steps;
    Array_dyn<s64> workflows;
    array_push_back(&workflows, 0);
    Hashmap<s64> id_to_workflow;
    
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
    
    auto read_int = [&]() {
        u64 val = 0;
        while ('0' <= arr[index] and arr[index] <= '9') {
            val = 10*val + (arr[index] - '0');
            ++index;
        }
        return val;
    };
    
    auto read_char = [&](u8 c) {
        if (arr[index] != c) {
            format_print("%a %a\n", Array_t<u8> {&c, 1}, array_subarray(arr, index, min(index+20, arr.size)));
        }
        assert(arr[index] == c);
        ++index;
    };
    
    while (arr[index] != '\n') {
        u64 workflow_id = read_id();
        read_char('{');
        
        while(arr[index] != '}') {
            Step s;
            s.property = read_id();
            switch (arr[index]) {
                case '<': ++index; s.cmp = 1; break;
                case '>': ++index; s.cmp = 2; break;
                default: s.cmp = 0; break;
            }
            if (s.cmp == 0) {
                s.next_step = s.property;
                s.property = 0;
            } else {
                s.value = read_int();
                read_char(':');
                s.next_step = read_id();
                read_char(',');
            }
            array_push_back(&steps, s);
        }
        read_char('}');
        read_char('\n');
        
        hashmap_set(&id_to_workflow, workflow_id, workflows.size-1);
        array_push_back(&workflows, steps.size);
    }
    
    read_char('\n');
    
    Hashmap<s64> props;
    s64 sum = 0;
    while (index < arr.size) {
        read_char('{');
        
        hashmap_clear(&props);
        
        while (arr[index] != '}') {
            u64 prop = read_id();
            read_char('=');
            s64 value = read_int();
            hashmap_set(&props, prop, value);
            
            if (arr[index] != '}') read_char(',');
            else break;
        }
        
        read_char('}');
        read_char('\n');
        
        s64 workflow = 'i' | 'n'<<8;
        while (true) {
            if (workflow == 'A' or workflow == 'R') break;
            s64 workflow_index = hashmap_get(&id_to_workflow, workflow);
            
            auto ws = array_subindex(workflows, steps, workflow_index);
            for (auto s: ws) {
                if (s.cmp) {
                    u64 p = hashmap_get(&props, s.property);
                    bool flag = s.cmp == 1 ? p < s.value : p > s.value;
                    if (not flag) continue;
                }
                workflow = s.next_step;
                break;
            }
        }
        
        if (workflow == 'A') {
            sum += hashmap_get(&props, 'x');
            sum += hashmap_get(&props, 'm');
            sum += hashmap_get(&props, 'a');
            sum += hashmap_get(&props, 's');
        }
    }
    
    
    format_print("%d\n", sum);
}