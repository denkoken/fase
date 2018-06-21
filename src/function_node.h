#ifndef FASE_FUNCTION_NODE_H_20180617
#define FASE_FUNCTION_NODE_H_20180617

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "exceptions.h"
#include "variable.h"

namespace fase {

class FunctionNode {
public:
    virtual FunctionNode *build(const std::vector<Variable *> &in_args) = 0;
    virtual void apply() = 0;
    void operator()() { apply(); }
};

template <typename... Args>
class StandardFunction : public FunctionNode {
public:

    ///
    /// Wrap a function which dose not return anything.
    ///
    template <class Callable>
    StandardFunction(Callable in_func) {
        func = std::function<void(Args...)>(in_func);
    }

    ///
    /// Build function with input arguments statically.
    ///  TODO: Is there any method to replace template VArgs with Args?
    ///
    template <typename... VArgs>
    FunctionNode *build(VArgs&... in_args) noexcept {
        static_assert(sizeof...(VArgs) == sizeof...(Args),
                      "The number of arguments is not matched");

        // Copy to an array of variables
        args = std::array<Variable, sizeof...(Args)>{in_args...};
        // Bind arguments
        binded = bind(std::index_sequence_for<Args...>());

        return this;
    }

    ///
    /// Build function with input arguments dynamically.
    ///
    FunctionNode *build(const std::vector<Variable *> &in_args) {
        if (in_args.size() != sizeof...(Args)) {
            throw(InvalidArgN(sizeof...(Args), in_args.size()));
        }

        // Copy to an array of variables
        for (int i = 0; i < int(sizeof...(Args)); i++) {
            args[i] = *in_args[i];
        }
        // Bind arguments
        binded = bind(std::index_sequence_for<Args...>());

        return this;
    }

    void apply() { binded(); }

private:
    template <std::size_t... Idx>
    auto bind(std::index_sequence<Idx...>) {
        return [&]() {
            func(*args[Idx]
                   .template getReader<
                       typename std::remove_reference<Args>::type>()...);
        };
    }

    std::function<void(Args...)> func;
    std::function<void()> binded;
    std::array<Variable, sizeof...(Args)> args;
};

}  // namespace fase

#endif  // FASE_FUNCTION_NODE_H_20180617
