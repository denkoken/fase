
#ifndef UTILS_H_20190215
#define UTILS_H_20190215

#include <algorithm>
#include <map>
#include <vector>

#include "variable.h"

namespace fase {

template <typename T, typename C>
inline bool exists(T& t, C& c) {
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

template <typename T>
inline void Extend(T&& a, T* b) {
    b->insert(std::begin(*b), std::begin(a), std::end(a));
}

template <typename Key, typename Val>
inline std::vector<Key> getKeys(const std::map<Key, Val>& map) {
    std::vector<Key> dst;
    for (auto& pair : map) {
        dst.emplace_back(std::get<0>(pair));
    }
    return dst;
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

class DependenceTree {
public:
    bool add(const std::string& depending, const std::string& depended) {
        if (checkLoop(depended, depending)) {
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

    std::vector<std::vector<std::string>> getDependenceLayer(
            const std::string& depending) const {
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

}  // namespace fase

#endif  // UTILS_H_20190215
