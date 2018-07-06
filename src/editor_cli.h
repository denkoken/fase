#ifndef EDITOR_CLI_H_20180628
#define EDITOR_CLI_H_20180628

#include "editor.h"

namespace fase {

class CLIEditor : public EditorBase<CLIEditor> {
public:
    template <typename T>
    bool addVarGenerator(const std::function<T(const std::string &)> &func) {
        var_generators[&typeid(T)] = func;
        return true;
    }

    void start(FaseCore *core);

    auto getVarGenerators() {
        return var_generators;
    }

private:
    // Variable generators
    std::map<const std::type_info *,
             std::function<Variable(const std::string &)>>
            var_generators;
};

}  // namespace fase

#endif  // EDITOR_CLI_H_20180628
