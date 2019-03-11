
#ifndef UNIV_FUNCTIONS_H_20190219
#define UNIV_FUNCTIONS_H_20190219

#include <iostream>

#include "common.h"

namespace fase {

template <typename... Args>
class UnivFuncGenerator {
public:
    template <typename Callable>
    static auto Gen(Callable&& f) {
        return Wrap(ToUniv(std::forward<Callable>(f),
                           std::index_sequence_for<Args...>()));
    }

private:
    template <std::size_t... Seq, typename Callable>
    static auto ToUniv(Callable&& f, std::index_sequence<Seq...>) {
        using Callable_N = std::decay_t<Callable>;
        if constexpr (std::is_copy_constructible_v<Callable_N>) {
            struct Dst {
                Callable_N f;
                void       operator()(std::vector<Variable>& args) {
                    f(static_cast<Args>(
                            *args[Seq].getWriter<std::decay_t<Args>>())...);
                }
            };
            return Dst{f};
        } else {
            auto fp = std::make_shared<Callable_N>(std::forward<Callable_N>(f));
            return [fp](std::vector<Variable>& args) {
                (*fp)(static_cast<Args>(
                        *args[Seq].getWriter<std::decay_t<Args>>())...);
            };
        }
    }

    template <typename Callable>
    static auto Wrap(Callable f) {
        struct Dst {
            Callable f;
            void     operator()(std::vector<Variable>& args, Report* preport) {
                if (args.size() != sizeof...(Args)) {
                    throw std::logic_error(
                            "Invalid size of variables at UnivFunc.");
                }
                if (preport != nullptr) {
                    using std::chrono::system_clock;
                    auto start_t = system_clock::now();
                    f(args);
                    preport->execution_time = system_clock::now() - start_t;
                } else {
                    f(args);
                }
            }
        };
        return Dst{f};
    }
};

template <typename Callable, typename... Args>
class HardFunc : public Callable {
    void operator()(Args&&... args) {
        std::vector<Variable> vs = {
                std::shared_ptr<Args>(std::forward<Args>(args))...};
        this->operator()(vs);
    }
};

}  // namespace fase

#endif  // UNIV_FUNCTIONS_H_20190219
