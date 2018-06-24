
#ifndef FASE_PIPELINE_EDITOR_H_20180622
#define FASE_PIPELINE_EDITOR_H_20180622

#include <memory>
#include <string>

#include "core.h"
#include "function_node.h"
#include "variable.h"

namespace fase {

namespace develop {

class Extension {
public:
    Extension(pe::FaseCore& core_,
              std::function<Variable(const std::string&, Variable)> use_e,
              std::function<void(std::unique_ptr<Extension>&&)> add_e)
        : core(&core_), useExtension(use_e) {}

    virtual Variable start(Variable) = 0;
    virtual std::string getName() const = 0;

protected:
    pe::FaseCore* core;
    std::function<Variable(const std::string&, Variable)> useExtension;
};

class Editor : public Extension {};
}  // namespace develop

inline namespace ui {
class PipelineEditor {
public:
    template <class EditorClass>
    PipelineEditor();

    void addInputVariable(const std::string& name, const Variable& val);

    template <typename... Args>
    void addFunction(const std::string& name, const FunctionBinder<Args...>& f,
                     const std::array<std::string, sizeof...(Args)>& argnames);

    void setInitFunc(const std::function<void()>& init_func);

    void startEditing();

private:
    pe::FaseCore core;
    std::unique_ptr<develop::Editor> editor;
    std::vector<std::unique_ptr<develop::Extension>> extensions;
};

}  // namespace ui

}  // namespace fase

#endif  // FASE_PIPELINE_EDITOR_H_20180622
