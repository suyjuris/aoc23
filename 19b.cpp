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
    
    struct Cube {
        u64 workflow;
        s64 cmin[4];
        s64 cmax[4];
        s64 count() {
            s64 count = 1;
            for (s64 i = 0; i < 4; ++i) {
                if (cmin[i] >= cmax[i]) return 0;
                count *= cmax[i] - cmin[i];
            }
            return count;
        }
    };
    
    auto subcube = [&](Cube c, u64 prop, s8 cmp, s64 value) {
        u8 pp;
        switch (prop) {
            case 'x': pp = 0; break;
            case 'm': pp = 1; break;
            case 'a': pp = 2; break;
            case 's': pp = 3; break;
            default: assert(false);
        }
        if (cmp == 1) {
            c.cmax[pp] = min(c.cmax[pp], value);
        } else if (cmp == 2) {
            c.cmin[pp] = max(c.cmin[pp], value+1);
        } else if (cmp == 3) {
            c.cmin[pp] = max(c.cmin[pp], value);
        } else if (cmp == 4) {
            c.cmax[pp] = min(c.cmax[pp], value+1);
        } else assert(false);
        return c;
    };
    
    auto print = [](Cube c) {
        return;
        format_print("x[%d,%d],m[%d,%d],a[%d,%d],s[%d,%d],%a\n",
                     c.cmin[0], c.cmax[0],
                     c.cmin[1], c.cmax[1], 
                     c.cmin[2], c.cmax[2], 
                     c.cmin[3], c.cmax[3], 
                     array_create_str((char*)&c.workflow));
    };
    
    Array_dyn<Cube> stack;
    array_push_back(&stack, {(u64)('i' | 'n'<<8), {1, 1, 1, 1}, {4001, 4001, 4001, 4001}});
    s64 sum = 0;
    while (stack.size) {
        Cube c = stack.back();
        --stack.size;
        
        if (c.workflow == 'A') {
            sum += c.count();
            continue;
        } else if (c.workflow == 'R') {
            continue;
        }
        
        s64 workflow_index = hashmap_get(&id_to_workflow, c.workflow);
        auto ws = array_subindex(workflows, steps, workflow_index);
        
        for (Step s: ws) {
            if (s.cmp) {
                Cube cc = subcube(c, s.property, s.cmp, s.value);
                cc.workflow = s.next_step;
                if (cc.count()) {
                    array_push_back(&stack, cc);
                    print(cc);
                }
                c = subcube(c, s.property, s.cmp+2, s.value);
                if (c.count() == 0) break;
            } else {
                c.workflow = s.next_step;
                array_push_back(&stack, c);
                print(c);
            }
        }
    }
    
    format_print("%d\n", sum);
}