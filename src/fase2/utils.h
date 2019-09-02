
#ifndef UTILS_H_20190215
#define UTILS_H_20190215

#include <algorithm>
#include <map>
#include <numeric>
#include <vector>

#include "variable.h"

namespace fase {

template <typename T, typename C>
inline bool exists(T&& t, C&& c) {
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

template <typename Container, class V>
inline void erase_all(Container& c, V&& v) {
    auto end = std::remove(c.begin(), c.end(), v);
    c.erase(end, c.end());
}

inline std::vector<std::string> split(const std::string& str, const char& sp) {
    std::size_t              start = 0;
    std::size_t              end;
    std::vector<std::string> dst;
    while (true) {
        end = str.find_first_of(sp, start);
        dst.emplace_back(std::string(str, start, end - start));
        start = end + 1;
        if (end >= str.size()) {
            break;
        }
    }
    return dst;
}

template <typename T>
inline void Extend(T&& a, T* b) {
    b->insert(std::end(*b), std::begin(a), std::end(a));
}
template <typename T>
inline void Extend(const T& a, T* b) {
    b->insert(std::end(*b), std::begin(a), std::end(a));
}

template <typename Key, typename Val>
inline std::vector<Key> getKeys(const std::map<Key, Val>& map) {
    std::vector<Key> dst;
    for (auto& pair : map) {
        dst.emplace_back(std::get<0>(pair));
    }
    return dst;
}

template <class Mutex, class Ptr>
inline void release(Mutex& m, Ptr& p) {
    p.reset();
    m.unlock();
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

inline void replace(const std::string& fr, const std::string& to,
                    std::string* str) {
    const size_t len = fr.length();
    for (size_t p = str->find(fr); p != std::string::npos; p = str->find(fr)) {
        *str = str->replace(p, len, to);
    }
}

inline std::string replace(std::string str, const std::string& fr,
                           const std::string& to) {
    const size_t len = fr.length();
    for (size_t p = str.find(fr); p != std::string::npos; p = str.find(fr)) {
        str = str.replace(p, len, to);
    }

    return str;
}

template <class HeadContainer, class... TailContainers>
inline bool CheckRepetition(HeadContainer&& head, TailContainers&&... tail) {
    for (auto& v : head) {
        auto counts = {std::count(std::begin(head), std::end(head), v),
                       std::count(std::begin(tail), std::end(tail), v)...};
        if (1 < std::accumulate(counts.begin(), counts.end(), 0)) {
            return true;
        }
    }
    if constexpr (sizeof...(tail) == 0) {
        return false;
    } else {
        return CheckRepetition(tail...);
    }
}

class DependenceTree {
public:
    bool add(const std::string& depending, const std::string& depended) {
        if (depending == depended || checkLoop(depended, depending)) {
            return false;
        }
        trees[depending][depended]++;
        return true;
    }
    void del(const std::string& depending, const std::string& depended) {
        trees[depending][depended]--;
        if (trees[depending][depended] <= 0) {
            trees[depending].erase(depended);
        }
    }

    std::vector<std::vector<std::string>>
    getDependenceLayer(const std::string& depending) const {
        if (isIndependent(depending)) return {};

        std::vector<std::vector<std::string>> dst;
        dst.emplace_back(getKeys(trees.at(depending)));

        for (auto& str : dst[0]) {
            auto deps = getDependenceLayer(str);
            dst.resize(std::max(deps.size() + 1, dst.size()));
            for (std::size_t i = 0; i < deps.size(); i++) {
                Extend(std::move(deps[i]), &dst[i + 1]);
            }
        }
        return dst;
    }

    bool isIndependent(const std::string& a) const {
        return !trees.count(a);
    }

    std::vector<std::string> getDependings(const std::string& a) const {
        return getKeys(trees.at(a));
    }

private:
    std::map<std::string, std::map<std::string, int>> trees;

    bool checkLoop(const std::string& a, const std::string& b) {
        for (auto& [n, c] : trees[a]) {
            if (b == n || checkLoop(n, b)) {
                return true;
            }
        }
        return false;
    }
};

} // namespace fase

#endif // UTILS_H_20190215
