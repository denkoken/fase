#ifndef FASE_H_20180617
#define FASE_H_20180617

#define NOMINMAX

#include "core.h"
#include "core_util.h"
#include "editor.h"
#include "type_utils.h"

#define FaseAddFunctionBuilder(fase, func, arg_types, arg_names, ...) \
    FaseAddFunctionBuilderImpl(fase, func, arg_types, arg_names, __VA_ARGS__)

#define FaseAddConstructAndEditor(fase, type, constructer, view_editor) \
    fase.registerConstructorAndVieweditor<type>(#type, constructer, view_editor)

namespace fase {

template <class Editor>
class Fase {
public:
    Fase() {
        SetupTypeUtils(&type_utils);
    };

    template <typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
            const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names = {},
            const std::array<Variable, sizeof...(Args)>& default_args = {}) {
        // Register to the core system
        return core.template addFunctionBuilder<Args...>(
                func_repr, func_val, arg_type_reprs, default_arg_reprs,
                arg_names, default_args);
    }

    template <typename T>
    bool registerTextIO(const std::string& name,
                        std::function<std::string(const T&)> serializer,
                        std::function<T(const std::string&)> deserializer);

    template <typename T>
    bool registerConstructorAndVieweditor(
            const std::string& name,
            std::function<std::string(const T&)> def_makers,
            std::function<std::unique_ptr<T>(const char*, const T&)>
                    view_editor);

    void setupEditor();

    template <typename... Args>
    bool runEditing(Args&&... args) {
        assert(editor);  // call setupEditor() brefore this function.
        return editor->run(std::forward<Args>(args)...);
    }

private:
    FaseCore core;
    std::unique_ptr<Editor> editor;

    TypeUtils type_utils;

    std::vector<std::tuple<const std::type_info*, VarEditor>> var_editor_buffer;
};

}  // namespace fase

#include "fase_impl.h"

#endif  // FASE_H_20180617
