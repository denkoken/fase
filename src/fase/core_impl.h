#ifndef FASE_CORE_IMPL_H_20180622
#define FASE_CORE_IMPL_H_20180622

#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "core.h"
#include "core_util.h"
#include "variable.h"

namespace fase {
namespace {

template <typename Conrainer>
bool exists(const Conrainer& vec, const typename Conrainer::value_type& key) {
    return std::find(vec.begin(), vec.end(), key) != std::end(vec);
}

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <std::size_t N>
auto CompleteDefaultArgs(int, const std::array<Variable, N>& src_args,
                         std::array<Variable, N>& dst_args, size_t idx = 0)
        -> bool {
    (void)src_args, (void)dst_args, (void)idx;
    return true;
}

template <std::size_t N, typename Head, typename... Tail>
auto CompleteDefaultArgs(bool, const std::array<Variable, N>& src_args,
                         std::array<Variable, N>& dst_args, size_t idx = 0)
        -> bool {
    if (src_args[idx].template isSameType<Head>()) {
        // Use input argument
        dst_args[idx] = src_args[idx];
    } else if (src_args[idx].template isSameType<void>()) {
        // Create by default constructor
        return false;
    } else {
        // Invalid type
        return false;
    }

    // Recursive call
    return CompleteDefaultArgs<N, Tail...>(0, src_args, dst_args, idx + 1);
}

template <std::size_t N, typename Head, typename... Tail>
auto CompleteDefaultArgs(int, const std::array<Variable, N>& src_args,
                         std::array<Variable, N>& dst_args, size_t idx = 0)
        -> decltype(typename std::remove_reference<Head>::type(), bool()) {
    if (src_args[idx].template isSameType<Head>()) {
        // Use input argument
        dst_args[idx] = src_args[idx];
    } else if (src_args[idx].template isSameType<void>()) {
        // Create by default constructor
        using Type = typename std::remove_reference<Head>::type;
        dst_args[idx] = Type();
    } else {
        // Invalid type
        return false;
    }

    // Recursive call
    return CompleteDefaultArgs<N, Tail...>(0, src_args, dst_args, idx + 1);
}

template <typename T>
const std::type_info* GetCleanTypeId() {
    return &typeid(typename std::remove_reference<
                   typename std::remove_cv<T>::type>::type);
}

template <typename T>
constexpr bool IsInputArgType() {
    using rrT = typename std::remove_reference<T>::type;
    return std::is_same<rrT, T>() ||
           !std::is_same<typename std::remove_const<rrT>::type, rrT>() ||
           std::is_rvalue_reference<T>();
}

template <typename... Args>
void unpackDummy(Args&&...) {}

template <size_t... Seq, typename... Args>
void setInput_(Node& node, std::index_sequence<Seq...>, Args&&... args) {
    unpackDummy(
            *node.arg_values[Seq]
                     .getWriter<typename std::remove_reference<Args>::type>() =
                    std::forward<Args>(args)...);
}

}  // namespace

template <typename Ret, typename... Args>
bool FaseCore::addFunctionBuilder(
        const std::string&                              func_repr,
        const std::function<Ret(Args...)>&              func_val,
        const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
        const std::array<std::string, sizeof...(Args)>& arg_names,
        const std::array<Variable, sizeof...(Args)>&    default_args,
        const std::string&                              code) {
    if (exists(functions, func_repr)) {
        return false;
    }

    // Check input types
    std::array<Variable, sizeof...(Args)> args;
    if (!CompleteDefaultArgs<sizeof...(Args), Args...>(0, default_args, args)) {
        return false;
    }

    // Check input representations
    if (func_repr.empty()) {
        return false;
    }

    // Register
    functions[func_repr] = {
            std::make_shared<FunctionBuilder<Args...>>(func_val),
            std::vector<std::string>(default_arg_reprs.begin(),
                                     default_arg_reprs.end()),
            std::vector<std::string>(arg_names.begin(), arg_names.end()),
            std::vector<Variable>(args.begin(), args.end()),
            std::vector<const std::type_info*>{GetCleanTypeId<Args>()...},
            std::vector<bool>{IsInputArgType<Args>()...},
            code,
    };
    return true;
}

template <typename... Args>
void FaseCore::setInput(Args&&... args) {
    Node& input = getCurrentPipeline().nodes[InputNodeStr()];
    setInput_(input, std::make_index_sequence<sizeof...(Args)>(),
              std::forward<Args>(args)...);
}

template <typename T>
T FaseCore::getOutput(const std::string& node_name, const size_t& arg_idx,
                      const T& default_value) {
    if (!exists(output_variables, node_name)) {
        return default_value;
    }
    if (output_variables[node_name].size() <= arg_idx) {
        return default_value;
    }
    const Variable& v = output_variables[node_name][arg_idx];
    if (!v.isSameType<T>()) {
        return default_value;
    }
    return *v.getReader<T>();  // Copy
}

template <typename... Inputs>
ExportIntermediate<Inputs...> FaseCore::exportPipeline(
        bool parallel_exe) const {
    if (!checkSubPipelineDependencies()) {
        std::cerr << "[FaseCore::exportPipeline()] recursive depending of sub "
                     "pipelines is found."
                  << std::endl;
        return {};
    }

    // setup sub pipelines
    for (auto& pair : sub_pipeline_fbs) {
        std::get<1>(pair)->init(*this, sub_pipelines.at(std::get<0>(pair)));
    }

    auto& nodes = getCurrentPipeline().nodes;

    std::vector<std::function<void()>>           export_exes;
    std::map<std::string, std::vector<Variable>> local_variables;

    if (!BuildPipeline(nodes, functions, parallel_exe, true, &export_exes,
                       &local_variables, nullptr)) {
        std::cerr << "[FaseCore::exportPipeline()] export build failed."
                  << std::endl;
        return {};
    }

    return ExportIntermediate<Inputs...>(std::move(export_exes),
                                         std::move(local_variables));
}

template <typename... Args, size_t... Idx>
inline void LetL2R(std::index_sequence<Idx...>, std::vector<Variable>& vs,
                   Args&&... args) {
    auto dummy = [](Args&...) {};
    dummy(*vs[Idx].getWriter<Args>() = std::forward<Args>(args)...);
}

template <typename... Args, size_t... Idx>
inline void LetR2L(std::index_sequence<Idx...>, std::vector<Variable>& vs,
                   Args&... args) {
    auto dummy = [](Args&...) {};
    dummy(args = *vs[Idx].getWriter<Args>()...);
}

template <typename... Args, size_t... Idx>
inline std::tuple<Args...> ToTuple(std::index_sequence<Idx...>,
                                   std::vector<Variable>& vs) {
    return {*vs[Idx].getWriter<Args>()...};
}

template <typename... Inputs>
template <typename... Outputs>
bool ExportIntermediate<Inputs...>::check() {
    if (export_exes.size() == 0) {
        return false;
    }

    bool valid_type = true;
    for (size_t i = 0; i < sizeof...(Inputs); i++) {
        std::vector<bool> check = {local_variables[InputNodeStr()][i]
                                           .template isSameType<Inputs>()...};
        valid_type = valid_type && check[i];
    }
    for (size_t i = 0; i < sizeof...(Outputs); i++) {
        std::vector<bool> check = {local_variables[OutputNodeStr()][i]
                                           .template isSameType<Outputs>()...};
        valid_type = valid_type && check[i];
    }
    return valid_type;
}

template <typename... Inputs>
template <typename... Outputs>
std::function<std::tuple<Outputs...>(Inputs&&...)>
ExportIntermediate<Inputs...>::get() {
    if (!check<Outputs...>()) {
        std::cerr << "[ExportIntermediate::get()] a type checking is failed."
                  << std::endl;
        return [](Inputs && ...) -> std::tuple<Outputs...> {
            std::cerr << "a broken export pipe is called." << std::endl;
            return {};
        };
    }

    return [lvs = std::make_shared<
                    std::map<std::string, std::vector<Variable>>>(
                    std::move(local_variables)),
            fs = std::move(export_exes)](Inputs&&... args) {
        LetL2R(std::index_sequence_for<Inputs...>(), (*lvs)[InputNodeStr()],
               std::forward<Inputs>(args)...);
        for (auto& f : fs) {
            f();
        }
        return ToTuple<Outputs...>(std::index_sequence_for<Outputs...>(),
                                   (*lvs)[OutputNodeStr()]);
    };
}

template <typename... Inputs>
template <typename... Outputs>
std::function<void(Inputs&&..., Outputs*...)>
ExportIntermediate<Inputs...>::getp() {
    if (!check<Outputs...>()) {
        std::cerr << "[ExportIntermediate::getp()] a type checking is failed."
                  << std::endl;
        return [](Inputs&&..., Outputs*...) {
            std::cerr << "a broken export pipe is called." << std::endl;
        };
    }

    return [lvs = std::make_shared<
                    std::map<std::string, std::vector<Variable>>>(
                    std::move(local_variables)),
            fs = std::move(export_exes)](Inputs&&... args,
                                         Outputs*... outputs) {
        LetL2R(std::index_sequence_for<Inputs...>(), (*lvs)[InputNodeStr()],
               std::forward<Inputs>(args)...);
        for (auto& f : fs) {
            f();
        }
        LetR2L(std::index_sequence_for<Outputs...>(), (*lvs)[OutputNodeStr()],
               (*outputs)...);
    };
}

}  // namespace fase

#endif  // CORE_IMPL_H_20180622
