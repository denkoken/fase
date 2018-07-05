#ifndef FASE_H_20180617
#define FASE_H_20180617

#include "core.h"
#include "editor.h"
#include "exceptions.h"
#include "function_node.h"
#include "variable.h"

#define FaseAddFunctionBuilder(fase, func, arg_types, arg_names, ...) \
    FaseAddFunctionBuilderImpl(fase, func, arg_types, arg_names, __VA_ARGS__)

#define FaseCoreAddFunctionBuilder(core, func, arg_types, ...) \
    FaseCoreAddFunctionBuilderImpl(core, func, arg_types, __VA_ARGS__)

namespace fase {

class Fase {
public:
    Fase() : core() {}

    template <class EditorClass>
    void setEditor() {
        editor = std::make_unique<EditorClass>();
    }

    template <typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_val_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names,
            const std::array<Variable, sizeof...(Args)>& default_args = {}) {
        const bool core_ret =
                core.addFunctionBuilder(func_repr, func_val, arg_type_reprs,
                                        arg_val_reprs, default_args);
        return core_ret;
    }

    void startEditing() {
        editor->start(&core);
    }

private:
    FaseCore core;
    std::unique_ptr<Editor> editor;
};

}  // namespace fase

#include "fase_impl.h"

#endif  // FASE_H_20180617
