
#ifndef UNIV_FUNCTIONS_H_20190219
#define UNIV_FUNCTIONS_H_20190219

#include <iostream>

#include "common.h"

namespace fase {

template <typename... Args>
class UnivFuncGenerator {
public:
    template <typename FuncObjGenerator>
    static auto Gen(FuncObjGenerator&& f) {
        return Wrap(ToUniv(std::forward<FuncObjGenerator>(f),
                           std::index_sequence_for<Args...>()));
    }

private:
    template <std::size_t... Seq, typename FuncObjGenerator>
    static auto ToUniv(FuncObjGenerator&& fog, std::index_sequence<Seq...>) {
        struct Dst {
            Dst() = default;
#define COPY(a)                                                                \
    if (!a.f) {                                                                \
        f = a.fog();                                                           \
    } else {                                                                   \
        f = a.f;                                                               \
    }
            Dst(Dst& a) {
                COPY(a);
            }
            Dst(const Dst& a) {
                COPY(a);
            }
            Dst(Dst&&) = default;
            Dst& operator=(Dst& a) {
                COPY(a);
                return *this;
            }
            Dst& operator=(const Dst& a) {
                COPY(a);
                return *this;
            }
#undef COPY
            Dst& operator=(Dst&&) = default;

            virtual ~Dst() = default;

            void operator()(std::vector<Variable>& args) {
                f(static_cast<Args>(
                        *args[Seq].getWriter<std::decay_t<Args>>())...);
            }

            std::function<std::function<void(Args...)>()> fog;
            std::function<void(Args...)>                  f;
        };
        Dst dst;
        dst.fog = std::forward<FuncObjGenerator>(fog);
        return dst;
    }

    template <typename Callable>
    static auto Wrap(Callable f) {
        return [f = std::move(f)](std::vector<Variable>& args,
                                  Report*                preport) mutable {
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
        };
    }
};

} // namespace fase

#endif // UNIV_FUNCTIONS_H_20190219
