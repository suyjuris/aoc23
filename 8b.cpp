#include "lib/global.hpp"
#include "lib/os_linux.cpp"
#include "lib/format.cpp"
#include "lib/hashmap.cpp"
#include "lib/number.cpp"
#include "lib/algorithm.cpp"

struct Node {
    u64 name, left, right;
    u64 get(bool b) { return b ? right : left; };
};
u64 read_node_name(Array_t<u8> arr, s64* index) {
    u64 result = 0ull
        | (u64)(arr[*index]-'A') << 16
        | (u64)(arr[*index+1]-'A') << 8
        | (u64)(arr[*index+2]-'A')
        ;
    *index += 3;
    while (*index < arr.size and not ('A' <= arr[*index] and arr[*index] <= 'Z')) ++*index;
    return result;
}

s64 gcd(s64 a, s64 b) {
    while (true) {
        if (a < b) simple_swap(&a, &b);
        if (b == 0) return a;
        a %= b;
    }
}
s64 lcm(s64 a, s64 b) {
    return a / gcd(a,b) * b;
}

int main() {
    auto arr = array_load_from_file("input/8"_arr);
    
    Array_dyn<u8> inst;
    s64 index = 0;
    
    for (; arr[index] != '\n'; ++index) {
        array_push_back(&inst, arr[index] == 'R');
    }
    index += 2;
    
    Hashmap<Node> nodes;
    nodes.empty = -1;
    Array_dyn<u64> cur;
    while (index < arr.size) {
        Node node;
        node.name  = read_node_name(arr, &index);
        node.left  = read_node_name(arr, &index);
        node.right = read_node_name(arr, &index);
        hashmap_set(&nodes, node.name, node);
        if ((node.name & 0xff) == 0) array_push_back(&cur, node.name);
    }
    
    s64 count = 1;
    for (u64 c: cur) {
        s64 len = 0;
        while ((c&0xff) != 0x19) {
            for (bool b: inst) {
                c = hashmap_get(&nodes, c).get(b);
            }
            len += inst.size;
        }
        count = lcm(count, len);
    }
    format_print("%d\n", count);
}