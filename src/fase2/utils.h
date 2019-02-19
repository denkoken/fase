
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

template <typename Container, class Checker>
inline Container get_all_if(Container& c, Checker&& checker) {
    Container storeds;
    for (auto& v : c) {
        if (checker(v)) {
            storeds.emplace_back(v);
        }
    }
    return {std::move(storeds)};
}

inline void RefCopy(std::vector<Variable>& src, std::vector<Variable>* dst) {
    dst->clear();
    dst->resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        (*dst)[i] = src[i].ref();
    }
}

inline void RefCopy(std::vector<Variable>::iterator&& begin,
                    std::vector<Variable>::iterator&& end,
                    std::vector<Variable>*            dst) {
    dst->clear();
    dst->reserve(std::size_t(end - begin));
    for (auto it = begin; it != end; it++) {
        dst->emplace_back(it->ref());
    }
}

}  // namespace fase

#endif  // UTILS_H_20190215
