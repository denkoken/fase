#ifndef FASE_H_20180617
#define FASE_H_20180617

#include "exceptions.h"
#include "function_node.h"
#include "variable.h"
#include "core.h"

namespace fase {

namespace develop {

class Editor {
public:
    virtual Variable start(pe::FaseCore*, Variable) = 0;
protected:
    std::function<Variable(const std::string&, Variable)> useExtension;
};

}  // namespace develop

class Fase {
public:
    template <class EditorClass>
    Fase();

    void addInputVariable(const std::string& name, const Variable& val);

    template<typename T, typename... Args>
    void addVariableBuilder(const std::string& name, const Args&... args);

    template <typename... Args>
    void addFunctionBuilder(const std::string& name, const FunctionBinder<Args...>& f,
                            const std::array<std::string, sizeof...(Args)>& argnames);

    void setInitFunc(const std::function<void()>& init_func);

    void startEditing();

private:
    pe::FaseCore core;
    std::unique_ptr<develop::Editor> editor;
};

}  // namespace fase

#endif  // FASE_H_20180617
