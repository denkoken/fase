
#ifndef FASE_PIPELINE_EDITOR_H_20180622
#define FASE_PIPELINE_EDITOR_H_20180622

#include <string>

#include "core.h"
#include "function_node.h"
#include "gui.h"
#include "variable.h"

namespace fase {

inline namespace ui {
class PipelineEditor {
public:
    PipelineEditor();

    void addInputVariable(const std::string& name, const Variable& val);

    template <typename... Args>
    void addFunctionNode(const std::string& name,
                         const StandardFunction<Args...>& f);

    void startEditing();

private:
    pe::FaseCore core;
    pe::FaseGUI gui;
};

}  // namespace ui

}  // namespace fase

#endif  // FASE_PIPELINE_EDITOR_H_20180622
