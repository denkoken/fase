#ifndef FASE_CORE_IMPL_H_20180622
#define FASE_CORE_IMPL_H_20180622

namespace fase {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <std::size_t N>
bool CompleteDefaultArgs(const std::array<Variable, N>& src_args,
                         std::array<Variable, N>& dst_args, size_t idx) {
    (void)src_args, (void)dst_args, (void)idx;
    return true;
}

template <std::size_t N, typename Head, typename... Tail>
bool CompleteDefaultArgs(const std::array<Variable, N>& src_args,
                         std::array<Variable, N>& dst_args, size_t idx = 0) {
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
    return CompleteDefaultArgs<N, Tail...>(src_args, dst_args, idx + 1);
}

template <typename... Args>
bool FaseCore::addFunctionBuilder(
        const std::string& func_repr,
        const std::function<void(Args...)>& func_val,
        const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
        const std::array<std::string, sizeof...(Args)>& arg_val_reprs,
        const std::array<std::string, sizeof...(Args)>& arg_names,
        const std::array<Variable, sizeof...(Args)>& default_args) {
    if (exists(functions, func_repr)) {
        return false;
    }

    // Check input types
    std::array<Variable, sizeof...(Args)> args;
    if (!CompleteDefaultArgs<sizeof...(Args), Args...>(default_args, args)) {
        return false;
    }

    // Check input representations
    if (func_repr.empty()) {
        return false;
    }
    for (auto&& arg_type_repr: arg_type_reprs) {
        if (arg_type_repr.empty()) {
            return false;
        }
    }

    // Register
    functions[func_repr] = {
        std::make_unique<FunctionBuilder<Args...>>(func_val),
        std::vector<std::string>(arg_type_reprs.begin(), arg_type_reprs.end()),
        std::vector<std::string>(arg_val_reprs.begin(), arg_val_reprs.end()),
        std::vector<std::string>(arg_names.begin(), arg_names.end()),
        std::vector<Variable>(args.begin(), args.end())};
    return true;
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
