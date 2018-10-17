#ifndef FASE_IMPL_H_20180705
#define FASE_IMPL_H_20180705
#include <cassert>
#include <sstream>

#include "fase.h"

#if defined(FASE_USE_ADD_FUNCTION_BUILDER_MACRO) && \
        defined(__cpp_if_constexpr) && defined(__cpp_inline_variables)
#include "auto_fb_adder.h"
#else
#define FaseAutoAddingFunctionBuilder(func_name, code) code
#endif

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
inline void extractArgExprs(std::string types,
                            std::array<std::string, N>& reprs) {
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

template <class... Parts>
inline Fase<Parts...>::Fase()
    : Parts(type_utils)..., core(std::make_shared<FaseCore>()) {
    SetupTypeUtils(&type_utils);
#if defined(FASE_USE_ADD_FUNCTION_BUILDER_MACRO) && \
        defined(__cpp_if_constexpr) && defined(__cpp_inline_variables)
    for (auto& builder_adder : for_macro::FuncNodeStorer::func_builder_adders) {
        builder_adder(core.get());
    }
#endif
}

template <class... Parts>
template <typename T>
inline bool Fase<Parts...>::registerTextIO(
        const std::string& name,
        std::function<std::string(const T&)>&& serializer,
        std::function<T(const std::string&)>&& deserializer,
        std::function<std::string(const T&)>&& def_maker) {
    type_utils.serializers[name] = [serializer](const Variable& v) {
        return serializer(*v.getReader<T>());
    };

    type_utils.deserializers[name] =
            [deserializer = std::forward<decltype(deserializer)>(deserializer)](
                    Variable& v, const std::string& str) {
                v.create<T>(deserializer(str));
            };

    type_utils.checkers[name] = [](const Variable& v) {
        return v.isSameType<T>();
    };

    type_utils.names[&typeid(T)] = name;

    type_utils.def_makers[name] =
            [def_maker = std::forward<decltype(def_maker)>(def_maker)](
                    const Variable& v) { return def_maker(*v.getReader<T>()); };
    return true;
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

#define FaseAddFunctionBuilder(app, func, arg_types, arg_names, ...) \
    FaseAddFunctionBuilderImpl(app, func, arg_types, arg_names, __VA_ARGS__)

/** @def FaseRegisterTestIO
 * @brief
 *      wrapper of Fase::registerTestIO().
 * @see Fase::registerTestIO()
 */
#define FaseRegisterTestIO(app, type, serializer, deserializer, constructer) \
    app.registerTextIO<type>(#type, serializer, deserializer, constructer)

}  // namespace fase

#endif  // CORE_IMPL_H_20180705
