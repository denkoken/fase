#ifndef FASE_H_20180617
#define FASE_H_20180617

#include "core.h"
#include "editor.h"

#define FaseAddFunctionBuilder(fase, func, arg_types, arg_names, ...) \
    FaseAddFunctionBuilderImpl(fase, func, arg_types, arg_names, __VA_ARGS__)

namespace fase {

template <class Editor>
class Fase {
public:
    Fase() {}

    template <typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_val_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names = {},
            const std::array<Variable, sizeof...(Args)>& default_args = {}) {
        // Register to the core system
        return core.template addFunctionBuilder<Args...>(
                func_repr, func_val, arg_type_reprs, arg_val_reprs, arg_names,
                default_args);
    }

    template <typename Gen>
    bool addVarGenerator(Gen &&gen) {
        return editor.addVarGenerator(std::forward<Gen>(gen));
    }

    void startEditing() {
        editor.start(&core);
    }

private:
    FaseCore core;
    Editor editor;
};

}  // namespace fase

#include "fase_impl.h"

#endif  // FASE_H_20180617
