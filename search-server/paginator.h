#pragma once

#include "document.h"

template <typename It> 
class Paginator {

public:

    Paginator() = default;

    Paginator(It range_begin, It range_end, size_t page_size) : page_size_(page_size) {
        size_t a = 0;
        std::vector<Document> v;
        for (auto it = range_begin; it != range_end; ++it) {
            v.push_back(*it);
            ++a;
            if (a == page_size_ || it == range_end - 1) {
                documents_.push_back(v);
                v.clear();
                a = 0;
            }
        }
    } 

    auto begin() const {
        return documents_.begin();
    }

    auto end() const {
        return documents_.end();
    }

private:
    std::vector<std::vector<Document>> documents_;
    size_t page_size_;
}; 

template <typename Container> 
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}