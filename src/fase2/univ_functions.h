
#ifndef UNIV_FUNCTIONS_H_20190219
#define UNIV_FUNCTIONS_H_20190219

#include <iostream>

#include "common.h"

namespace fase {

template <typename CallForm>
class UnivFuncGenerator {};

template <typename Ret, typename... Args>
class UnivFuncGenerator<Ret(Args...)> {
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
    if (a.fog) {                                                               \
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
                if constexpr (std::is_same_v<Ret, void>) {
                    f(static_cast<Args>(*Get<Args>(args[Seq]))...);
                } else {
                    args[sizeof...(Args)].assignedAs(
                            f(static_cast<Args>(*Get<Args>(args[Seq]))...));
                }
            }

            std::function<std::function<Ret(Args...)>()> fog;
            std::function<Ret(Args...)>                  f;
        };
        Dst dst;
        dst.fog = std::forward<FuncObjGenerator>(fog);
        dst.f = dst.fog();
        return dst;
    }

    template <typename Callable>
    static auto Wrap(Callable f) {
        return [f = std::move(f)](std::vector<Variable>& args,
                                  Report*                preport) mutable {
            if (args.size() !=
                sizeof...(Args) + size_t(!std::is_same_v<Ret, void>)) {
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

    template <typename Type>
    static auto Get(Variable& v) {
        if constexpr (IsInputType<Type>()) {
            return v.getReader<std::decay_t<Type>>();
        } else {
            return v.getWriter<std::decay_t<Type>>();
        }
    }
};

} // namespace fase

#endif // UNIV_FUNCTIONS_H_20190219
