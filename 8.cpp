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
    while (index < arr.size) {
        Node node;
        node.name  = read_node_name(arr, &index);
        node.left  = read_node_name(arr, &index);
        node.right = read_node_name(arr, &index);
        hashmap_set(&nodes, node.name, node);
    }
    
    s64 count = 0;
    u64 cur = 0;
    while (cur != 0x191919) {
        for (bool b: inst) {
            cur = hashmap_get(&nodes, cur).get(b);
            count += 1;
        }
    }
    
    format_print("%d\n", count);
}