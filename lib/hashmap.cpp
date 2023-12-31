
#ifdef HASHMAP_TEST
#include <unistd.h>
#include "global.hpp"
#endif

#ifdef HASHMAP_TEST
#define HASHMAP_BASE_SIZE 2
#define assert_nofuzz(x) (void)(x)
#else
#define HASHMAP_BASE_SIZE 16
#define assert_nofuzz(x) assert((x))
#endif

#ifdef HASHMAP_STATS
#define HASHMAP_STAT_ONLY(x) x
#else
#define HASHMAP_STAT_ONLY(x)
#endif


template <typename T>
struct Hashmap {
    static constexpr u64 _SLOT_FIND_MASK = 1ull << 63;
    struct Slot {
        u64 key;
        T val;
        HASHMAP_STAT_ONLY(u64 count;)
    };

    u64 empty = 0;
    Array_t<Slot> slots;
    s64 size = 0;

    HASHMAP_STAT_ONLY(
        s64 collisions = 0;
        s64 lookups = 0;
    )
};

s64 _hashmap_slot_base(u64 map_slots_size, u64 key) {
#ifndef HASHMAP_TEST
    u64 x = key;
    x ^= (x >> 23) ^ (x >> 51);
    x *= 0x9e6c63d0676a9a99ull;
    x ^= (x >> 23) ^ (x >> 51);
	//u64 x = key + 0x9e3779b97f4a7c15;
	//x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	//x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	//x =  x ^ (x >> 31);
    return x & (map_slots_size-1);
#else
    return key & (map_slots_size-1); // no hashing for the fuzzer
#endif
}

template <typename T>
s64 _hashmap_slot_find(Hashmap<T>* map, u64 key, bool* out_empty) {
    assert(key != map->empty);
    assert(out_empty);
    HASHMAP_STAT_ONLY(++map->lookups;)

    if (map->slots.size == 0) {
        *out_empty = true;
        return -1;
    }
    
    s64 slot_base = _hashmap_slot_base(map->slots.size, key);
    for (s64 i = 0; ; ++i) {
        s64 slot = (slot_base + i) & (map->slots.size-1);
        s64 slot_key = map->slots.data[slot].key;
        HASHMAP_STAT_ONLY(++map->slots.data[slot].count;)
        if (slot_key == key) {
            *out_empty = false;
            return slot;
        } else if (slot_key == map->empty) {
            *out_empty = true;
            return slot;
        }
        HASHMAP_STAT_ONLY(++map->collisions;)
    }
    
    assert_false;
    *out_empty = false;
    return -1;
}

template <typename T>
void _hashmap_enlarge(Hashmap<T>* map) {
    if (map->slots.size == 0) {
        // Initialise to size HASHMAP_BASE_SIZE
        map->slots = array_create<typename Hashmap<T>::Slot>(HASHMAP_BASE_SIZE);
        for (auto& slot: map->slots) slot.key = map->empty;
        assert(map->size == 0);
    } else {
        array_resize(&map->slots, map->slots.size * 2);
        for (auto& slot: array_subarray(map->slots, map->slots.size/2)) slot.key = map->empty;
        
        for (s64 i = 0; i < map->slots.size; ++i) {
            typename Hashmap<T>::Slot slot = map->slots[i];
            if (slot.key == map->empty) {
                if (i >= map->slots.size/2) break;
                continue;
            }
            map->slots[i].key = map->empty;
            bool index_empty;
            s64 index = _hashmap_slot_find(map, slot.key, &index_empty);
            assert(index_empty);
            map->slots[index] = slot;
        }
    }
}

template <typename T, typename V>
void hashmap_set(Hashmap<T>* map, u64 key, V val_) {
    assert_nofuzz(key != map->empty);
    if (key == map->empty) return;
    if (map->size * 5 >= map->slots.size * 3) _hashmap_enlarge(map);

    T val = val_;
    bool slot_empty;
    s64 slot = _hashmap_slot_find(map, key, &slot_empty);
    map->slots[slot].key = key;
    map->slots[slot].val = val;
    map->size += slot_empty;
    
}

template <typename T>
T* hashmap_getptr(Hashmap<T>* map, u64 key) {
    assert_nofuzz(key != map->empty);
    if (key == map->empty) return nullptr;
    if (map->size * 5 >= map->slots.size * 3) _hashmap_enlarge(map);

    bool slot_empty;
    s64 slot = _hashmap_slot_find(map, key, &slot_empty);
    if (slot_empty) return nullptr;

    return &map->slots.data[slot].val;
}

template <typename T, typename V>
void hashmap_setnew(Hashmap<T>* map, u64 key, V val_) {
    assert_nofuzz(key != map->empty);
    if (key == map->empty) return;
    if (map->size * 5 >= map->slots.size * 3) _hashmap_enlarge(map);
    T val = val_;

    bool slot_empty;
    s64 slot = _hashmap_slot_find(map, key, &slot_empty);
    assert_nofuzz(slot_empty);
    map->slots[slot].key = key;
    map->slots[slot].val = val;
    map->size += slot_empty;
}

template <typename T>
T hashmap_get(Hashmap<T>* map, u64 key) {
    T* ptr = hashmap_getptr(map, key);
    assert(ptr);
    return *ptr;
}

template <typename T>
T* hashmap_getcreate(Hashmap<T>* map, u64 key, T init={}) {
    assert_nofuzz(key != map->empty);
    if (key == map->empty) return nullptr;
    if (map->size * 5 >= map->slots.size * 3) _hashmap_enlarge(map);

    bool slot_empty;
    s64 slot = _hashmap_slot_find(map, key, &slot_empty);
    if (slot_empty) {
        map->slots[slot].key = key;
        map->slots[slot].val = init;
        ++map->size;
    }
    
    return &map->slots[slot].val;
}

template <typename T>
bool hashmap_delete_if_present(Hashmap<T>* map, u64 key, T* out_value=nullptr) {
    assert_nofuzz(key != map->empty);
    if (key == map->empty) return false;
    
    bool slot_empty;
    s64 slot = _hashmap_slot_find(map, key, &slot_empty);
    if (slot_empty) return false;
    if (out_value) *out_value = map->slots[slot].val;

    s64 empty = slot;
    for (s64 i = 1; i < map->slots.size; ++i) {
        s64 slot_i = (slot + i) & (map->slots.size-1);
        s64 key_i = map->slots[slot_i].key;
        if (key_i == map->empty) break;
        s64 slot_base = _hashmap_slot_base(map->slots.size, key_i) & (map->slots.size-1);
        if (((empty - slot_base) & (map->slots.size-1)) <= ((slot_i - slot_base) & (map->slots.size-1))) {
            map->slots[empty] = map->slots[slot_i];
            empty = slot_i;
        }
    }
    map->slots[empty].key = map->empty;
    --map->size;
    
    return true;
}

template <typename T>
T hashmap_delete(Hashmap<T>* map, u64 key) {
    T result {};
    bool not_empty = hashmap_delete_if_present(map, key, &result);
    assert_nofuzz(not_empty);
    return result;
}

template <typename T>
void hashmap_clear(Hashmap<T>* map) {
    for (s64 i = 0; i < map->slots.size; ++i) {
        map->slots[i].key = map->empty;
    }
    map->size = 0;
}

template <typename T>
void hashmap_set_empty(Hashmap<T>* map, u64 key_empty) {
    map->empty = key_empty;
    hashmap_clear(map);
}

template <typename T>
void hashmap_free(Hashmap<T>* map) {
    array_free(&map->slots);
    map->size = 0;
}


static inline u64 _hash_rotate_left(u64 x, int k) {
	return (x << k) | (x >> (64 - k));
}

u64 hash_u64(u64 x) {
    // see http://mostlymangling.blogspot.com/2020/01/nasam-not-another-strange-acronym-mixer.html
    x ^= _hash_rotate_left(x, 39) ^ _hash_rotate_left(x, 17);
    x *= 0x9e6c63d0676a9a99ull;
    x ^= (x >> 23) ^ (x >> 51);
    x *= 0x9e6d62d06f6a9a9bull;
    return x ^ (x >> 23) ^ (x >> 51);
}

u64 _hash_u64_nofuzz(u64 x) {
#ifndef HASHMAP_TEST
    x *= 0x9e6c63d0676a9a99ull;
    x ^= (x >> 23) ^ (x >> 51);
    return x;
    //x ^= _hash_rotate_left(x, 39) ^ _hash_rotate_left(x, 17);
    //x *= 0x9e6c63d0676a9a99ull;
    //x ^= (x >> 23) ^ (x >> 51);
    //x *= 0x9e6d62d06f6a9a9bull;
    //return x ^ (x >> 23) ^ (x >> 51);
#else
    return x;
#endif
}

u64 hash_u64_pair(u64 a, u64 b) {
    return hash_u64(hash_u64(a) ^ b);
}

u64 hash_str(Array_t<u8> str) {
    u64 x = 0xffdf38dd3e69bd91ull ^ str.size;
    s64 i = 0;
    for (; i+8 <= str.size; i += 8) {
        u64 a = *(u64*)(str.data + i);
        x = hash_u64(a ^ x);
    }
    for (s64 j = i; j < str.size; ++j) {
        x ^= str.data[j] << 8*j;
    }
    return hash_u64(x);
}

u64 hash_arr(Array_t<u64> arr) {
    u64 x = 0xefdf38dd3e69bd91ull ^ arr.size;
    for (u64 a: arr) x = hash_u64(a ^ x);
    return x;
}


#ifdef HASHMAP_TEST

template <typename T>
struct Hashmap_stupid {
    struct Slot { u64 key; T value; };
    Array_dyn<Slot> slots;
};

template <typename T, typename V>
void hashmap_set(Hashmap_stupid<T>* map, u64 key, V value_) {
    T value = value_;
    for (auto& i: map->slots) {
        if (i.key == key) {
            i.value = value;
            return;
        }
    }
    array_push_back(&map->slots, {key, value});
}

template <typename T>
bool hashmap_setnew(Hashmap_stupid<T>* map, u64 key, T value) {
    for (auto i: map->slots) {
        if (i.key == key) return false;
    }
    array_push_back(&map->slots, {key, value});
    return true;
}

template <typename T>
bool hashmap_delete(Hashmap_stupid<T>* map, u64 key, T value) {
    for (s64 i = 0; i < map->slots.size; ++i) {
        if (map->slots[i].key == key) {
            map->slots[i] = map->slots.back();
            --map->slots.size;
            return true;
        }
    }
    return false;
}

template <typename T>
T* hashmap_getptr(Hashmap_stupid<T>* map, u64 key) {
    for (auto& i: map->slots) {
        if (i.key == key) {
            return &i.value;
        }
    }
    return nullptr;
}

template <typename T>
u64 hash_hashmap(Hashmap_stupid<T>* map) {
    u64 h = hash_u64(map->slots.size);
    for (auto i: map->slots) {
        h += hash_u64_pair(i.key, i.value);
    }
    return h;
}

template <typename T>
void hashmap_free(Hashmap_stupid<T>* map) {
    array_free(&map->slots);
}


template <typename T>
u64 hash_hashmap(Hashmap<T>* map) {
    u64 h = hash_u64(map->size);
    for (auto i: map->slots) {
        if (i.key != map->empty) {
            h += hash_u64_pair(i.key, i.val);
        }
    }
    return h;
}

u64 hash_hashmap(Hashmap_u32* map) {
    u64 h = hash_u64(map->size);
    for (s64 it = 0; it < map->slots.size; ++it) {
        auto i = map->slots[it];
        for (s64 j = 0; j < 12; ++j) {
            if (i.hashes[j&1] >> (j>>1)*10) {
                h += hash_u64_pair(map->slot_keys[it].keys[j], i.values[j]);
            }
        }
    }
    return h;
}

void _hashmap_selftest(Hashmap_u32* map) {
    assert(map->slots.size == map->slot_keys.size);
    for (s64 it = 0; it < map->slots.size; ++it) {
        auto i = map->slots[it];
        s64 count = 0;
        for (s64 j = 0; j < 12; ++j) {
            u64 subhash = (i.hashes[j&1] >> (j>>1)*10) & 0x3ff;
            if (subhash) {
                ++count;
                u64 subhash2 = _hash_u64_nofuzz(map->slot_keys[it].keys[j]) & 0x3ff;
                subhash2 += not subhash2;
                assert(subhash == subhash2);
            }
        }
        assert(count == i.hashes[0] >> 60);
    }
}

void _hashmap_print(Hashmap_u32* map) {
    printf("size: %lld, slots: %lld\n", map->size, map->slots.size);
    for (s64 it = 0; it < map->slots.size; ++it) {
        auto i = map->slots[it];
        printf("%03lld: size=%lld\n", it, i.hashes[0] >> 60);
        printf("subhash  ");
        for (s64 j = 0; j < 12; ++j) {
            printf("  %03llx ", (i.hashes[j&1] >> (j>>1)*10) & 0x3ff);
        }
        printf("\nkey      ");
        for (s64 j = 0; j < 12; ++j) {
            printf("%05llx ", map->slot_keys[it].keys[j]);
        }
        printf("\nvalue    ");
        for (s64 j = 0; j < 12; ++j) {
            printf("   %02x ", i.values[j]);
        }
        puts("\n");
    }
}

#ifndef HASHMAP_VERBOSE
#define HASHMAP_VERBOSE 0
#endif

void _hashmap_fuzz(char* buf, int len) {
    Hashmap<u32> map;
    Hashmap_u32 map2;
    Hashmap_stupid<u32> map_stupid;
    const bool v = HASHMAP_VERBOSE;
    map.empty = 0xf1234567ull;
    for (int i = 0; i+2 < len; i += 3) {
        u32 val = (u64)buf[i+1] << 12 | buf[i+2];
        if (buf[i] % 2 == 0) {
            if (v) printf("set %02x %02x\n", val, buf[i]);
            hashmap_set(&map, val, buf[i]);
            hashmap_set(&map2, val, buf[i]);
            hashmap_set(&map_stupid, val, buf[i]);
        } else {
            if (v) printf("getptr %02x\n", val);
            u8 flags = 0;
            flags |= hashmap_getptr(&map, val) ? 1 : 0;
            flags |= hashmap_getptr(&map2, val) ? 2 : 0;
            flags |= hashmap_getptr(&map_stupid, val) ? 4 : 0;
            assert(flags == 0 or flags == 7);
        }
        if (v) _hashmap_print(&map2);
        _hashmap_selftest(&map2);
        u64 h1 = hash_hashmap(&map);
        u64 h2 = hash_hashmap(&map2);
        u64 h3 = hash_hashmap(&map_stupid);
        assert(h1 == h3 and h2 == h3);
    }
    hashmap_free(&map);
    hashmap_free(&map2);
    hashmap_free(&map_stupid);
}

#ifndef __AFL_FUZZ_TESTCASE_LEN
  ssize_t fuzz_len;
  #define __AFL_FUZZ_TESTCASE_LEN fuzz_len
  char fuzz_buf[4096];
  #define __AFL_FUZZ_TESTCASE_BUF fuzz_buf
  #define __AFL_FUZZ_INIT() void sync(void);
  #define __AFL_LOOP(x) ((fuzz_len = read(0, fuzz_buf, sizeof(fuzz_buf))) > 0 ? 1 : 0)
  #define __AFL_INIT() sync() 
#endif


__AFL_FUZZ_INIT();

int main() {

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    char *buf = (char*)__AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 4096) continue;
        _hashmap_fuzz(buf, len);
    }

    return 0;
}

#endif
