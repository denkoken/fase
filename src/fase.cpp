#include "fase.h"

#include <memory>

namespace fase {

template <class EditorClass>
Fase::Fase() : core() {
    editor = std::make_unique<EditorClass>();
}

template<typename T, typename... Args>
void Fase::addVariableBuilder(const std::string& name, const Args&... args) {
    core.addVariableBuilder(name, [args...]() -> Variable { return T(args...); });
}

template <typename... Args>
void Fase::addFunctionBuilder(
    const std::string& name, const FunctionBinder<Args...>& f,
    const std::array<std::string, sizeof...(Args)>& argnames) {
    core.addFunctionBuilder(
        name, std::make_unique<FunctionBinder<Args...>>(f),
        std::vector<std::string>(std::begin(argnames), std::end(argnames)));
}

void Fase::startEditing() { editor->start(&core, true); }

}  // namespace fase
