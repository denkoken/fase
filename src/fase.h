#ifndef FASE_H_20180617
#define FASE_H_20180617

#include "core.h"
#include "editor.h"
#include "exceptions.h"
#include "function_node.h"
#include "variable.h"

#define faseAddFunctionBuilder(fase, func, arg_names, ...)                 \
    fase.addFunctionBuilder(#func, std::function<void(__VA_ARGS__)>(func), \
                            arg_names)

namespace fase {

class Fase {
public:
    Fase() : core() {}

    template <class EditorClass>
    void setEditor() {
        editor = std::make_unique<EditorClass>();
    }

    template <typename... Args>
    void addFunctionBuilder(
            const std::string& name, std::function<void(Args...)>&& f,
            const std::array<std::string, sizeof...(Args)>& arg_names) {
        // core.addFunctionBuilder<Args...>(name, std::move(f));

        (void)arg_names;
        // editor->addFunctionBuilder();
    };

    void startEditing() { editor->start(&core); }

private:
    FaseCore core;
    std::unique_ptr<Editor> editor;
};

}  // namespace fase

#endif  // FASE_H_20180617
