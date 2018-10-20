#ifndef FASE_FUNCTION_NODE_H_20180617
#define FASE_FUNCTION_NODE_H_20180617

#include <array>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "exceptions.h"
#include "variable.h"

namespace fase {

struct ResultReport {
    using TimeType = decltype(std::chrono::system_clock::now() -
                              std::chrono::system_clock::now());
    TimeType execution_time;  // sec
};

class FunctionBuilderBase {
public:
    virtual ~FunctionBuilderBase() {}
    virtual std::function<void()> build(
            const std::vector<Variable*>& in_args) = 0;
    virtual std::function<void()> build(const std::vector<Variable*>& in_args,
                                        ResultReport* report) = 0;
};

template <typename... Args>
class FunctionBuilder : public FunctionBuilderBase {
public:
    ///
    /// Wrap a function which dose not return anything.
    ///
    template <class Callable>
    FunctionBuilder(Callable in_func) {
        func = std::function<void(Args...)>(in_func);
    }

    ///
    /// Build function with input arguments statically.
    ///
    template <typename... VArgs>
    std::function<void()> build(VArgs*... in_args) noexcept {
        static_assert(sizeof...(VArgs) == sizeof...(Args),
                      "The number of arguments is not matched");

        // Copy to an array of variables
        std::array<Variable*, sizeof...(Args)> args = {in_args...};
        // Bind arguments
        return bind(args, std::index_sequence_for<Args...>());
    }

    ///
    /// Build function with input arguments dynamically.
    ///
    std::function<void()> build(const std::vector<Variable*>& in_args) {
        if (in_args.size() != sizeof...(Args)) {
            throw(InvalidArgN(sizeof...(Args), in_args.size()));
        }

        // Copy to an array of variables
        std::array<Variable*, sizeof...(Args)> args;
        for (size_t i = 0; i < sizeof...(Args); i++) {
            args[i] = in_args[i];
        }
        // Bind arguments
        return bind(args, std::index_sequence_for<Args...>());
    }

    std::function<void()> build(const std::vector<Variable*>& in_args,
                                ResultReport* report) {
        if (in_args.size() != sizeof...(Args)) {
            throw(InvalidArgN(sizeof...(Args), in_args.size()));
        }

        // Copy to an array of variables
        std::array<Variable*, sizeof...(Args)> args;
        for (size_t i = 0; i < sizeof...(Args); i++) {
            args[i] = in_args[i];
        }
        // Bind arguments
        return bind(args, std::index_sequence_for<Args...>(), report);
    }

private:
    template <std::size_t... Idx>
    auto bind(std::array<Variable*, sizeof...(Args)>& args,
              std::index_sequence<Idx...>) {
        return [=]() {
            func(*(*args[Idx])
                          .template getReader<typename std::remove_reference<
                                  Args>::type>()...);
        };
    }

    template <std::size_t... Idx>
    auto bind(std::array<Variable*, sizeof...(Args)>& args,
              std::index_sequence<Idx...>, ResultReport* report) {
        return [=]() {
            auto start = std::chrono::system_clock::now();
            func(*(*args[Idx])
                          .template getReader<typename std::remove_reference<
                                  Args>::type>()...);
            report->execution_time = std::chrono::system_clock::now() - start;
        };
    }

    std::function<void(Args...)> func;
};

}  // namespace fase

#endif  // FASE_FUNCTION_NODE_H_20180617
