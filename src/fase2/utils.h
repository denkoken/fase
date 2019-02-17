
#ifndef UTILS_H_20190215
#define UTILS_H_20190215

#include <algorithm>
#include <vector>

#include "variable.h"

namespace fase {

template <typename T, typename C>
inline bool exsists(T& t, C& c) {
    return std::end(c) != std::find(std::begin(c), std::end(c), t);
}

inline void RefCopy(std::vector<Variable>& src, std::vector<Variable>* dst) {
    dst->clear();
    dst->resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        (*dst)[i] = src[i].ref();
    }
}

}  // namespace fase

#endif  // UTILS_H_20190215
