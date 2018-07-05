#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"

namespace fase {

template <typename P>
class EditorBase {
public:
    virtual ~EditorBase() {}

    template <typename... Args>
    bool addFunctionBuilder() {
        return static_cast<P*>(this)->template addFunctionBuilder<Args...>();
    }

    virtual void start(FaseCore*) {}
};

class CLIEditor : public EditorBase<CLIEditor> {
public:
    template <typename... Args>
    bool addFunctionBuilder();

    void start(FaseCore* core);

private:
    std::map<std::string,
             std::function<Variable(const std::string&)>> var_generators;
};

}  // namespace fase

#include "cli_editor_impl.h"

#endif  // EDITOR_H_20180628
