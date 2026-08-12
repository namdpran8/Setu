#pragma once

#include <vector>

namespace android {

template <typename T>
class Vector : public std::vector<T> {
public:
    ssize_t push(const T& val) { this->push_back(val); return this->size() - 1; }
    ssize_t add(const T& val) { this->push_back(val); return this->size() - 1; }
    const T& itemAt(size_t index) const { return (*this)[index]; }
    T& editItemAt(size_t index) { return (*this)[index]; }
    bool isEmpty() const { return this->empty(); }
    void appendVector(const Vector<T>& vector) {
        for (size_t i = 0; i < vector.size(); i++) this->push_back(vector[i]);
    }
    ssize_t insertAt(const T& item, size_t index, size_t numItems = 1) {
        this->insert(this->begin() + index, numItems, item);
        return index;
    }
    ssize_t insertAt(size_t index, size_t numItems = 1) {
        this->insert(this->begin() + index, numItems, T());
        return index;
    }
};

} // namespace android
