#include "fase.h"

#include <memory>

namespace fase {

template <class EditorClass>
Fase::Fase() : core() {
    editor = std::make_unique<EditorClass>();
}

template<typename T, typename... Args>
void Fase::addVariableBuilder(const std::string& name, const Args&... args) {
    core.addVariableBuilder<T>(name, [args...]() -> Variable { return T(args...); });
}

template <typename... Args, class Callable>
void Fase::addFunctionBuilder(const std::string& name, Callable f,
                        const std::array<std::string, sizeof...(Args)>& argnames){
    core.addFunctionBuilder<Args...>(name, f, argnames);
};

void Fase::startEditing() { editor->start(&core, true); }

}  // namespace fase
