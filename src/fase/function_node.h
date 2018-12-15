#ifndef FASE_FUNCTION_NODE_H_20180617
#define FASE_FUNCTION_NODE_H_20180617

#include <array>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "exceptions.h"
#include "variable.h"

namespace fase {
struct ResultReport {
    using TimeType = decltype(std::chrono::system_clock::now() -
                              std::chrono::system_clock::now());
    TimeType execution_time;  // sec

    std::map<std::string, ResultReport> child_reports;
};

class FunctionBuilderBase {
public:
    virtual ~FunctionBuilderBase() {}
    virtual std::function<void()> build(
            const std::vector<Variable*>& in_args) const = 0;
    virtual std::function<void()> build(const std::vector<Variable*>& in_args,
                                        ResultReport* report) const = 0;
    virtual std::function<void()> buildNoTypeCheckAtRun(
            const std::vector<Variable*>& in_args) const = 0;
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
    std::function<void()> build(VArgs*... in_args) const noexcept {
        static_assert(sizeof...(VArgs) == sizeof...(Args),
                      "The number of arguments is not matched");

        // Copy to an array of variables
        std::array<Variable*, sizeof...(Args)> args{{in_args...}};
        // Bind arguments
        return bind(args, std::index_sequence_for<Args...>(), nullptr);
    }

    ///
    /// Build function with input arguments dynamically.
    ///
    std::function<void()> build(const std::vector<Variable*>& in_args) const {
        return build0(in_args);
    }

    std::function<void()> build(const std::vector<Variable*>& in_args,
                                ResultReport*                 report) const {
        return build0(in_args, report);
    }

    std::function<void()> buildNoTypeCheckAtRun(
            const std::vector<Variable*>& in_args) const {
        if (in_args.size() != sizeof...(Args)) {
            throw(InvalidArgN(sizeof...(Args), in_args.size()));
        }
        if (!typeCheck(in_args, std::index_sequence_for<Args...>())) {
            std::cerr << "Invalid Type Cast at FunctionBuilder::build()."
                      << std::endl;
            return [] {};
        }

        return optimizedBind(in_args, std::index_sequence_for<Args...>());
    }

private:
    std::function<void()> build0(const std::vector<Variable*>& in_args,
                                 ResultReport* report = nullptr) const {
        if (in_args.size() != sizeof...(Args)) {
            throw(InvalidArgN(sizeof...(Args), in_args.size()));
        }
        if (!typeCheck(in_args, std::index_sequence_for<Args...>())) {
            std::cerr << "Invalid Type Cast at FunctionBuilder::build()."
                      << std::endl;
            return [] {};
        }

        // Copy to an array of variables
        std::array<Variable*, sizeof...(Args)> args;
        for (size_t i = 0; i < sizeof...(Args); i++) {
            args[i] = in_args[i];
        }
        // Bind arguments
        return bind(args, std::index_sequence_for<Args...>(), report);
    }

    template <std::size_t... Idx>
    std::function<void()> bind(std::array<Variable*, sizeof...(Args)>& args,
                               std::index_sequence<Idx...>,
                               ResultReport* report) const {
        if (report == nullptr) {
            return [=]() {
                func(*(*args[Idx])
                              .template getReader<
                                      typename std::remove_reference<
                                              Args>::type>()...);
            };
        } else {
            return [=]() {
                auto start = std::chrono::system_clock::now();
                func(*(*args[Idx])
                              .template getReader<
                                      typename std::remove_reference<
                                              Args>::type>()...);
                report->execution_time =
                        std::chrono::system_clock::now() - start;
            };
        }
    }

    template <std::size_t... Idx>
    bool typeCheck(const std::vector<Variable*>& in_args,
                   std::index_sequence<Idx...>) const {
        std::initializer_list<bool> results = {
                in_args[Idx]->isSameType<Args>()...};
        for (const bool& result : results) {
            if (!result) return false;
        }
        return true;
    }

    template <std::size_t... Idx>
    auto optimizedBind(const std::vector<Variable*>& args,
                       std::index_sequence<Idx...>) const {
        std::tuple<typename std::remove_reference<Args>::type*...>
                var_pointers = std::make_tuple(
                        (*args[Idx])
                                .template getWriter<
                                        typename std::remove_reference<
                                                Args>::type>()
                                .get()...);
        return [=] { func(*std::get<Idx>(var_pointers)...); };
    }

    std::function<void(Args...)> func;
};

}  // namespace fase

#endif  // FASE_FUNCTION_NODE_H_20180617
