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

#ifdef FASE_USE_ADD_FUNCTION_BUILDER_MACRO

class FuncNodeStorer {
public:
    template <typename Ret, typename... Args>
    FuncNodeStorer(std::string func_name, const std::string& code,
                   Ret (*fp)(Args...)) {
        while (true) {
            if (codes.count(func_name)) {
                func_name += "_";
                continue;
            }
            break;
        }

        std::array<std::string, sizeof...(Args)> arg_type_reprs;
        std::array<std::string, sizeof...(Args)> default_arg_reprs;
        std::array<std::string, sizeof...(Args)> arg_names;
        std::array<Variable, sizeof...(Args)> default_args;
        // TODO initialize these varialbes with code.

        codes[func_name] = code;
        func_builder_adders.push_back([=](auto *app) {
            std::clog << "add " << func_name << std::endl;
            app->addFunctionBuilder(func_name,
                                    std::function<Ret(Args...)>(fp),
                                    arg_type_reprs,
                                    default_arg_reprs,
                                    arg_names,
                                    default_args);
        });
    }

    inline static std::vector<std::function<void(Fase<GUIEditor>*)>> func_builder_adders;
    inline static std::map<std::string, std::string> codes;
};

#ifdef __COUNTER__

#define FaseAutoAddingFunctionBuilder__(func_name, code, c) \
    code \
    static fase::FuncNodeStorer FaseFuncNodeStorer__ ## func_name ## c (#func_name, #code, func_name);
#define FaseAutoAddingFunctionBuilder_(func_name, code, c) FaseAutoAddingFunctionBuilder__(func_name, code, c)
#define FaseAutoAddingFunctionBuilder(func_name, code) FaseAutoAddingFunctionBuilder_(func_name, code, __COUNTER__)

#else // ifdef __COUNTER__

#define FaseAutoAddingFunctionBuilder(func_name, code) \
    var static fase::FuncNodeStorer FaseFuncNodeStorer__ ## func_name (#func_name, code, func_name);

#endif

#else
#define TEST_MACRO(func_name, code) code
#endif

template <class Editor>
void Fase<Editor>::setupEditor() {

#ifdef FASE_USE_ADD_FUNCTION_BUILDER_MACRO
    for (auto& builder_adder : FuncNodeStorer::func_builder_adders) {
        builder_adder(this);
    }
#endif

    editor = std::make_unique<Editor>(&core, type_utils);

    for (auto& pair : var_editor_buffer) {
        editor->addVarEditor(std::get<0>(pair),
                             std::move(std::get<1>(pair)));
    }
    var_editor_buffer.clear();
}

template <class Editor>
template <typename T>
bool Fase<Editor>::registerTextIO(const std::string& name,
                    std::function<std::string(const T&)> serializer,
                    std::function<T(const std::string&)> deserializer) {
    type_utils.serializers[name] = [serializer](const Variable& v) {
        return serializer(*v.getReader<T>());
    };

    type_utils.deserializers[name] =
            [deserializer](Variable& v, const std::string& str) {
                v.create<T>(deserializer(str));
            };
}

template <class Editor>
template <typename T>
bool Fase<Editor>::registerConstructorAndVieweditor(
        const std::string& name,
        std::function<std::string(const T&)> def_makers,
        std::function<std::unique_ptr<T>(const char*, const T&)>
                view_editor) {
    type_utils.def_makers[name] = [def_makers](const Variable& v) {
        return def_makers(*v.getReader<T>());
    };

    var_editor_buffer.push_back(
            {&typeid(T),
             [f = view_editor](const char* label, const Variable& v) {
                 auto p = f(label, *v.getReader<T>());
                 if (p) return Variable(std::move(*p));
                 return Variable();
             }});

    type_utils.checkers[name] = [](const Variable& v) {
        return v.isSameType<T>();
    };

    type_utils.names[&typeid(T)] = name;
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

#define FaseAddFunctionBuilder(fase, func, arg_types, arg_names, ...) \
    FaseAddFunctionBuilderImpl(fase, func, arg_types, arg_names, __VA_ARGS__)

#define FaseAddConstructAndEditor(fase, type, constructer, view_editor) \
    fase.registerConstructorAndVieweditor<type>(#type, constructer, view_editor)

}  // namespace fase

#endif  // CORE_IMPL_H_20180705
