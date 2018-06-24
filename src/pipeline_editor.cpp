
#include "pipeline_editor.h"

#include <memory>

namespace fase {

inline namespace ui {

template <class EditorClass>
PipelineEditor::PipelineEditor() : core() {
    auto use_extension = [this](const std::string& name, Variable arg) -> Variable {
        for (auto& ext : extensions) {
            if (ext->getName() == name) {
                return ext->start(arg);
            }
        }
    };
    editor = std::make_unique<EditorClass>(core, use_extension);
    editor->addExtensions(&extensions);
}

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

void PipelineEditor::startEditing() { editor->start(true); }

}  // namespace ui

}  // namespace fase
