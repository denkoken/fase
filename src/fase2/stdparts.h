
#ifndef STDPARTS_H_20190318
#define STDPARTS_H_20190318

#include <tuple>

#include "parts_base.h"

// =============================================================================
// =========================== User Interface ==================================
// =============================================================================

namespace fase {

class ExportableParts : public PartsBase {
public:
    inline ExportedPipe exportPipe() const;
};

template <typename... RetTypes>
class ToHard {
public:
    template <typename... Args>
    class Pipe {
    public:
        template <class Exported>
        static inline auto Gen(Exported&& exported);
    };

private:
    template <std::size_t... Seq>
    static std::tuple<RetTypes...> Wrap(std::vector<Variable*>& ret_ps,
                                        std::index_sequence<Seq...>) {
        return std::make_tuple(
                std::move(*ret_ps[Seq]->getWriter<RetTypes>())...);
    }
};

class CallableParts : public PartsBase {
public:
    inline bool call(std::vector<Variable>& args);

    inline bool call(const std::string&     pipeline_name,
                     std::vector<Variable>& args);
};

template <typename... ReturnTypes>
class HardCallableParts : public PartsBase {
public:
    template <typename... Args>
    inline std::tuple<ReturnTypes...> callHard(Args&&... args);
};

} // namespace fase

// =============================================================================
// =========================== Non User Interface ==============================
// =============================================================================

namespace fase {

inline ExportedPipe ExportableParts ::exportPipe() const {
    auto [guard, pcm] = getAPI()->getReader();
    return pcm->exportPipe(pcm->getFocusedPipeline());
}

inline bool CallableParts::call(std::vector<Variable>& args) {
    auto [guard, pcm] = getAPI()->getWriter();
    return (*pcm)[pcm->getFocusedPipeline()].call(args);
}

inline bool CallableParts::call(const std::string&     pipeline_name,
                                std::vector<Variable>& args) {
    auto [guard, pcm] = getAPI()->getWriter();
    return (*pcm)[pipeline_name].call(args);
}

template <typename... RetTypes>
template <typename... Args>
template <class Exported>
inline auto ToHard<RetTypes...>::Pipe<Args...>::Gen(Exported&& exported) {
    class Dst {
    public:
        std::tuple<RetTypes...> operator()(Args... args) {
            std::vector<Variable> arg_vs = {
                    std::make_unique<std::decay_t<Args>>(
                            std::forward<Args>(args))...};
            [[maybe_unused]] auto _ = {
                    arg_vs.emplace_back(typeid(RetTypes))...};
            if (!soft(arg_vs)) {
                throw std::runtime_error(
                        "HardExportPipe : input/output type isn't "
                        "match!");
            }
            std::vector<Variable*> ret_ps;
            for (std::size_t i = arg_vs.size() - sizeof...(RetTypes);
                 i < arg_vs.size(); i++) {
                ret_ps.emplace_back(&arg_vs[i]);
            }
            return Wrap(ret_ps, std::index_sequence_for<RetTypes...>());
        }
        void reset() {
            soft.reset();
        }
        std::decay_t<Exported> soft;
    };
    if (!exported) {
        throw std::runtime_error("HardExportPipe : Empty pipeline was wraped");
    }
    return Dst{std::forward<Exported>(exported)};
}

template <typename... ReturnTypes>
template <typename... Args>
inline std::tuple<ReturnTypes...>
HardCallableParts<ReturnTypes...>::callHard(Args&&... args) {
    struct Dum {
        HardCallableParts<ReturnTypes...>* that;
        void                               reset() {}
                                           operator bool() {
            return true;
        }
        bool operator()(std::vector<Variable>& vs) {
            auto [guard, pcm] = that->getAPI()->getWriter();
            return (*pcm)[pcm->getFocusedPipeline()].call(vs);
        }
    };
    return ToHard<ReturnTypes...>::template Pipe<Args...>::Gen(Dum{this})(
            std::forward<Args>(args)...);
}

} // namespace fase

#endif // STDPARTS_H_20190318
