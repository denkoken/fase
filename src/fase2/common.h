
#ifndef COMMON_H_20190217
#define COMMON_H_20190217

#include <functional>
#include <string>
#include <vector>

#include "variable.h"

namespace fase {

using UnivFunc = std::function<void(std::vector<Variable>&)>;

template <typename... Args>
class UnivFuncGenerator {
public:
    template <typename Callable>
    static auto Gen(Callable&& f) {
        return Wrap(std::forward<Callable>(f),
                    std::index_sequence_for<Args...>());
    }

private:
    template <std::size_t... Seq, typename Callable>
    static auto Wrap(Callable&& f, std::index_sequence<Seq...>) {
        auto fp = std::make_shared<std::decay_t<Callable>>(
                std::forward<std::decay_t<Callable>>(f));
        return [fp](std::vector<Variable>& args) {
            if (args.size() != sizeof...(Args)) {
                throw std::logic_error(
                        "Invalid size of variables at UnivFunc.");
            }
            (*fp)(static_cast<Args>(
                    *args[Seq].getWriter<std::decay_t<Args>>())...);
        };
    }
};

struct Node {
    std::string           func_name;
    std::vector<Variable> args;
    int                   priority;
};

struct Link {
    std::string src_node;
    std::size_t src_arg;
    std::string dst_node;
    std::size_t dst_arg;
};

}  // namespace fase

#endif  // COMMON_H_20190217
