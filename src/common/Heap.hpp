#pragma once

#include <vector>
#include <cassert>


namespace common {

template<typename T>
class MinHeap {
public:

    void insert(T value) {
        elements.push_back(value);
        size_t i = elements.size() - 1;
        
        while(i != 0 && elements[i] < elements[parent(i)]) {
            swap(i, parent(i));
            i = parent(i);
        }
    }

    auto peek() -> T {
        return elements[0];
    }

    auto extract_min() -> T {
        assert(elements.size() != 0);

        T min = elements[0];
        elements[0] = elements[elements.size() - 1];
        elements.pop_back();
        heapify(0);

        return min;
    }

    void remove(T value) {
        for(size_t i = 0; i < elements.size(); i++) {
            if(elements[i] == value) {
                elements[i] = elements[elements.size() - 1];
                elements.pop_back();
                heapify(i);
                break;
            }
        }
    }

    void clear() {
        elements.clear();
    }

    auto size() -> size_t {
        return elements.size();
    }

    auto empty() -> bool {
        return elements.size() == 0;
    }

private:

    constexpr auto parent(size_t i) -> size_t {
        return (i - 1) / 2;
    }

    constexpr auto left(size_t i) -> size_t {
        return (i * 2) + 1;
    }

    constexpr auto right(size_t i) -> size_t {
        return (i * 2) + 2;
    }

    void swap(size_t a, size_t b) {
        T temp = elements[a];
        elements[a] = elements[b];
        elements[b] = temp;
    }

    void heapify(size_t i) {
        size_t min = i;
        size_t l = left(i);
        size_t r = right(i);

        if(l < elements.size() && elements[l] < elements[min]) {
            min = l;
        }
        
        if(r < elements.size() && elements[r] < elements[min]) {
            min = r;
        }

        if(min != i) {
            swap(i, min);
            heapify(min);
        }
    }

    std::vector<T> elements;
};

} //namespace common