
#include "pipeline_editor.h"

#include <memory>

namespace fase {

inline namespace ui {
template <class EditorClass>
PipelineEditor::PipelineEditor() : core() {
    auto use_extension = [this](const std::string& name,
                                Variable arg) -> Variable {
        for (auto& ext : extensions) {
            if (ext->getName() == name) {
                return ext->start(arg);
            }
        }
    };
    auto add_extension = [this](std::unique_ptr<develop::Extension>&& ext) {
        auto extension = std::forward<std::unique_ptr<develop::Extension>>(ext);
        std::string name = extension->getName();
        for (auto& ext_ : extensions) {
            if (ext_->getName() == name) return;
        }
        extensions.emplace_back(std::move(extension));
    };
    editor = std::make_unique<EditorClass>(core, use_extension, add_extension);
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
