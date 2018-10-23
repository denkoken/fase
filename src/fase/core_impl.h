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
template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <typename T>
inline bool exists(const std::set<T>& set, const T& key) {
    return set.find(key) != std::end(set);
}

template <typename T>
bool exists(const std::vector<T>& vec, const T& key) {
    return std::find(vec.begin(), vec.end(), key) != std::end(vec);
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
    unpackDummy(*node.arg_values[Seq].getWriter<Args>() = args...);
}

}  // namespace

template <typename Ret, typename... Args>
bool FaseCore::addFunctionBuilder(
        const std::string& func_repr,
        const std::function<Ret(Args...)>& func_val,
        const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
        const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
        const std::array<std::string, sizeof...(Args)>& arg_names,
        const std::array<Variable, sizeof...(Args)>& default_args) {
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
    for (auto&& arg_type_repr : arg_type_reprs) {
        if (arg_type_repr.empty()) {
            return false;
        }
    }

    // Register
    functions[func_repr] = {
            std::make_shared<FunctionBuilder<Args...>>(func_val),
            std::vector<std::string>(arg_type_reprs.begin(),
                                     arg_type_reprs.end()),
            std::vector<std::string>(default_arg_reprs.begin(),
                                     default_arg_reprs.end()),
            std::vector<std::string>(arg_names.begin(), arg_names.end()),
            std::vector<Variable>(args.begin(), args.end()),
            std::vector<const std::type_info*>{GetCleanTypeId<Args>()...},
            std::vector<bool>{IsInputArgType<Args>()...},
    };
    return true;
}

template <typename... Args>
void FaseCore::setInput(Args&&... args) {
    Node& input = pipelines[editting_pipeline].nodes[InputNodeStr()];
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

}  // namespace fase

#endif  // CORE_IMPL_H_20180622
