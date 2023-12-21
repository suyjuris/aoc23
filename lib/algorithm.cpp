
template <typename T, typename Func>
void array_sort_insertion(Array_t<T>* arr, Func&& key) {
    for (s64 i = 1; i < arr->size; ++i) {
        T i_el = (*arr)[i];
        auto i_key = key(i_el);
        
        s64 target = i;
        while (target > 0 and key((*arr)[target-1]) > i_key) --target;
        
        for (s64 j = i; j > target; --j) {
            (*arr)[j] = (*arr)[j-1];
        }
        (*arr)[target] = i_el;
    }
}
template <typename T>
void array_sort_insertion(Array_t<T>* arr) {
    for (s64 i = 1; i < arr->size; ++i) {
        T i_el = (*arr)[i];
        
        s64 target = i;
        while (target > 0 and (*arr)[target-1] > i_el) --target;
        
        for (s64 j = i; j > target; --j) {
            (*arr)[j] = (*arr)[j-1];
        }
        (*arr)[target] = i_el;
    }
}

// key must return integers in the range [0, key_upper)
template <typename T, typename Func>
void array_sort_counting(Array_t<T>* arr, Func&& key, s64 key_upper) {
    Array_t<s64> counts = array_create<s64>(key_upper);
    Array_t<T> temp = array_create<T>(arr->size);
    defer {
        array_free(&counts);
        array_free(&temp);
    };
    
    for (T i: *arr) ++counts[key(i)];
    
    s64 sum = 0;
    for (s64 i = 0; i < key_upper; ++i) {
        s64 tmp = counts[i]; counts[i] = sum; sum += tmp;
    }
    
    for (T i: *arr) temp[counts[key(i)]++] = i;
    array_memcpy(arr, temp);
}

template <typename T>
void array_reverse(Array_t<T>* arr) {
    for (s64 i = 0; 2*i < arr->size-1; ++i) {
        simple_swap(&(*arr)[i], &(*arr)[arr->size-1 - i]);
    }
}

template <typename T, typename Func>
struct Heap {
    Array_dyn<T> arr;
    Func func;
};

template <typename T, typename Func>
Heap<T, Func> heap_init(Func&& func) {
    return Heap<T, Func> {Array_dyn<T> {}, func};
}

template <typename T, typename Func>
void _heap_bubble_down(Array_t<T> heap, s64 i, Func&& func) {
    auto i_key = func(heap[i]);
    while (i*2+1 < heap.size) {
        s64 i0 = i*2+1;
        s64 i1 = i0 + (i0+1 < heap.size);
        auto i0_key = func(heap[i0]);
        auto i1_key = func(heap[i1]);
        bool imb = i0_key < i1_key;
        s64 im = imb ? i1 : i0;
        auto im_key = imb ? i1_key : i0_key;
        if (i_key < im_key) {
            simple_swap(&heap[i], &heap[im]);
            i = im;
        } else break;
    }
}

template <typename T, typename Func>
T heap_pop(Heap<T, Func>* heap) {
    T result = heap->arr[0];
    heap->arr[0] = heap->arr.back();
    --heap->arr.size;
    if (heap->arr.size) {
        _heap_bubble_down(heap->arr, 0, heap->func);
    }
    return result;
}

template <typename T, typename Func>
void heap_push(Heap<T, Func>* heap, T el) {
    array_push_back(&heap->arr, el);
    s64 i = heap->arr.size-1;
    auto i_val = heap->func(el);
    while (i > 0) {
        s64 ip = (i-1) / 2;
        auto ip_val = heap->func(heap->arr[ip]);
        if (ip_val < i_val) {
            simple_swap(&heap->arr[i], &heap->arr[ip]);
            i = ip;
        } else break;
    }
}

template <typename T, typename Func>
void array_sort_heap(Array_t<T>* arr, Func&& key) {
    for (s64 i = arr->size/2 - 1; i >= 0; --i) {
        _heap_bubble_down(*arr, i, key);
    }
    for (s64 i = arr->size-1; i > 0; --i) {
        simple_swap(&(*arr)[0], &(*arr)[i]);
        auto arr_tmp = *arr;
        arr_tmp.size = i;
        _heap_bubble_down(arr_tmp, 0, key);
    }
}
template <typename T>
void array_sort_heap(Array_t<T>* arr) {
    array_sort_heap(arr, [](T a) { return a; });
}

namespace Array_merge_generic {
    enum Flags: u8 {
        ONLY_A = 1, ONLY_B = 2, BOTH = 4,
        UNION = ONLY_A | ONLY_B | BOTH,
        INTERSECTION = BOTH,
        DIFFERENCE = ONLY_A,
        SYMMETRIC_DIFFERENCE = ONLY_A | ONLY_B
    };
}

template <typename T>
void array_merge_generic(Array_dyn<T>* into, Array_t<T> a, Array_t<T> b, u8 flags) {
    s64 i = 0, j = 0;
    while (i < a.size and j < b.size) {
        auto x = a[i]; auto y = b[j];
        if (x < y and (flags & Array_merge_generic::ONLY_A)) {
            array_push_back(into, x); 
        }
        if (x > y and (flags & Array_merge_generic::ONLY_B)) {
            array_push_back(into, y);
        }
        if (x == y and (flags & Array_merge_generic::BOTH)) {
            array_push_back(into, x);
        }
        i += x <= y;
        j += x >= y;
    }
    if (flags & Array_merge_generic::ONLY_A) {
        array_append(into, array_subarray(a, i));
    }
    if (flags & Array_merge_generic::ONLY_B) {
        array_append(into, array_subarray(b, j));
    }
}

struct String_key {
    Array_t<u8> str;
    bool operator<  (String_key o) const { return array_cmp(str, o.str) <  0; }
    bool operator>  (String_key o) const { return array_cmp(str, o.str) >  0; }
    bool operator<= (String_key o) const { return array_cmp(str, o.str) <= 0; }
    bool operator>= (String_key o) const { return array_cmp(str, o.str) >= 0; }
    bool operator== (String_key o) const { return array_equal(str, o.str); }
    bool operator!= (String_key o) const { return not array_equal(str, o.str); }
};