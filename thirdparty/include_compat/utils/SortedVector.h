#pragma once

#include "Vector.h"
#include <algorithm>

namespace android {

template <typename T>
class SortedVector : public Vector<T> {
public:
    void add(const T& val) {
        auto it = std::lower_bound(this->begin(), this->end(), val);
        this->insert(it, val);
    }
    
    ssize_t indexOf(const T& val) const {
        auto it = std::lower_bound(this->begin(), this->end(), val);
        if (it != this->end() && *it == val) {
            return std::distance(this->begin(), it);
        }
        return -1;
    }
    
    void removeAt(size_t index) {
        this->erase(this->begin() + index);
    }
};

} // namespace android
