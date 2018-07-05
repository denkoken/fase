#ifndef FASE_IMPL_H_20180705
#define FASE_IMPL_H_20180705
#include <cassert>
#include <sstream>

namespace fase {

template <typename... Args>
struct NArgs {
    size_t N = sizeof...(Args);
};

template <typename... Args>
struct NArgs<void(Args...)> {
    size_t N = sizeof...(Args);
};

template <std::size_t N>
void extractArgExprs(std::string types, std::array<std::string, N>& reprs) {
    // Remove '(' and ')'
    auto l_par_idx = types.find('(');
    auto r_par_idx = types.rfind(')');
    if (l_par_idx != std::string::npos && r_par_idx != std::string::npos) {
        types = types.substr(l_par_idx + 1, r_par_idx - l_par_idx - 1);
    }

    // Split by ','
    std::stringstream ss(types);
    std::string item;
    size_t idx = 0;
    while (std::getline(ss, item, ',')) {
        // Remove extra spaces
        size_t l_sp_idx = 0;
        while (item[l_sp_idx] == ' ') l_sp_idx++;
        item = item.substr(l_sp_idx);
        size_t r_sp_idx = item.size() - 1;
        while (item[r_sp_idx] == ' ') r_sp_idx--;
        item = item.substr(0, r_sp_idx + 1);

        // Register
        reprs[idx] = item;
        idx += 1;
        assert(idx <= N);
    }
}

#define FaseExpandListHelper(...) \
    { __VA_ARGS__ }
#define FaseExpandList(v) FaseExpandListHelper v

#define FaseAddFunctionBuilderImpl(cls, func, arg_types, arg_names, ...)      \
    [&]() {                                                                   \
        std::array<std::string, fase::NArgs<void arg_types>{}.N>              \
                arg_type_reprs;                                               \
        fase::extractArgExprs(#arg_types, arg_type_reprs);                    \
        std::array<std::string, fase::NArgs<void arg_types>{}.N>              \
                default_arg_reprs;                                            \
        fase::extractArgExprs(#__VA_ARGS__, default_arg_reprs);               \
        return cls.addFunctionBuilder(                                        \
                #func, std::function<void arg_types>(func), arg_type_reprs,   \
                default_arg_reprs, FaseExpandList(arg_names), {__VA_ARGS__}); \
    }()

}  // namespace fase

#endif  // CORE_IMPL_H_20180705
