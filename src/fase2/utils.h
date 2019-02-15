
#ifndef UTILS_H_20190215
#define UTILS_H_20190215

#include <algorithm>

namespace fase {

template <typename T, typename C>
bool exsists(T& t, C& c) {
    return std::end(c) != std::find(std::begin(c), std::end(c), t);
}

}  // namespace fase

#endif  // UTILS_H_20190215
