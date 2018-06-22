
#include "pipeline_editor.h"

#include <memory>

namespace fase {

inline namespace ui {
PipelineEditor::PipelineEditor() : core(), gui(core) {}

void PipelineEditor::addInputVariable(const std::string& name,
                                      const Variable& val) {
    core.addVariable(name, val);
}

template <typename... Args>
void PipelineEditor::addFunction(
    const std::string& name, const FunctionBinder<Args...>& f,
    const std::array<std::string, sizeof...(Args)>& argnames) {
    core.addFunction(
        name, std::make_unique<FunctionBinder<Args...>>(f),
        std::vector<std::string>(std::begin(argnames), std::end(argnames)));
}

void PipelineEditor::startEditing() { gui.start(); }

}  // namespace ui

}  // namespace fase
