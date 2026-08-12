#pragma once

#include <vector>

namespace android {

template <typename KEY, typename VALUE>
class KeyedVector {
    std::vector<std::pair<KEY, VALUE>> mData;
public:
    ssize_t replaceValueFor(const KEY& key, const VALUE& value) {
        ssize_t idx = indexOfKey(key);
        if (idx >= 0) {
            mData[idx].second = value;
        } else {
            idx = add(key, value);
        }
        return idx;
    }
    
    ssize_t add(const KEY& key, const VALUE& val) {
        mData.push_back({key, val});
        return mData.size() - 1;
    }
    ssize_t add(const KEY& key) {
        mData.push_back({key, VALUE()});
        return mData.size() - 1;
    }
    
    size_t size() const { return mData.size(); }
    bool isEmpty() const { return mData.empty(); }
    
    ssize_t indexOfKey(const KEY& key) const {
        for (size_t i = 0; i < mData.size(); ++i) {
            if (mData[i].first == key) return (ssize_t)i;
        }
        return -1;
    }
    
    const VALUE& valueAt(size_t index) const { return mData[index].second; }
    VALUE& editValueAt(size_t index) { return mData[index].second; }
    const KEY& keyAt(size_t index) const { return mData[index].first; }
    
    const VALUE& operator[](size_t index) const { return mData[index].second; }
    
    VALUE valueFor(const KEY& key) const {
        ssize_t idx = indexOfKey(key);
        if (idx >= 0) return mData[idx].second;
        return VALUE();
    }
};
template <typename KEY, typename VALUE>
class DefaultKeyedVector : public KeyedVector<KEY, VALUE> {
public:
    DefaultKeyedVector() = default;
    DefaultKeyedVector(const VALUE& defValue) : mDefault(defValue) {}
    
    VALUE valueFor(const KEY& key) const {
        ssize_t idx = this->indexOfKey(key);
        return idx >= 0 ? this->valueAt(idx) : mDefault;
    }
private:
    VALUE mDefault = VALUE();
};

} // namespace android
