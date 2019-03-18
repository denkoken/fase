
#ifndef STDPARTS_H_20190318
#define STDPARTS_H_20190318

#include <tuple>

#include "parts_base.h"

namespace fase {

class ExportableParts : public PartsBase {
public:
    ExportedPipe exportPipe() const {
        auto [guard, pcm] = getReader();
        return pcm->exportPipe(pcm->getFocusedPipeline());
    }
};

class CallableParts : public PartsBase {
public:
    bool call(std::vector<Variable>& args) {
        auto [guard, pcm] = getWriter();
        return (*pcm)[pcm->getFocusedPipeline()].call(args);
    }

    bool call(const std::string& pipeline_name, std::vector<Variable>& args) {
        auto [guard, pcm] = getWriter();
        return (*pcm)[pipeline_name].call(args);
    }
};

template <typename... ReturmTypes>
class HardCallableParts : public PartsBase {
public:
    template <typename... Args>
    std::tuple<ReturmTypes...> callHard(Args&&... args) {
        std::vector<Variable> arg_vs = {std::make_unique<std::decay_t<Args>>(
                std::forward<Args>(args))...};
        [[maybe_unused]] auto _ = {arg_vs.emplace_back(typeid(ReturmTypes))...};

        {
            auto [guard, pcm] = getWriter();
            if (!(*pcm)[pcm->getFocusedPipeline()].call(arg_vs)) {
                throw std::runtime_error(
                        "HardCallableParts : input/output type isn't match!");
            }
        }
        std::vector<Variable*> ret_ps;
        for (std::size_t i = arg_vs.size() - sizeof...(ReturmTypes);
             i < arg_vs.size(); i++) {
            ret_ps.emplace_back(&arg_vs[i]);
        }
        return wrap(ret_ps, std::index_sequence_for<ReturmTypes...>());
    }

private:
    template <std::size_t... Seq>
    std::tuple<ReturmTypes...> wrap(std::vector<Variable*>& ret_ps,
                                    std::index_sequence<Seq...>) {
        return std::make_tuple(
                std::move(*ret_ps[Seq]->getWriter<ReturmTypes>())...);
    }
};

}  // namespace fase

#endif  // STDPARTS_H_20190318
