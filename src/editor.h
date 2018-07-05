#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"

namespace fase {

template <typename P>
class EditorBase {
public:
    template <typename Gen>
    bool addVarGenerator(Gen &&) {}
    void start(FaseCore *) {}
};

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

class GUIEditor : public EditorBase<GUIEditor> {
public:
    void start(FaseCore *core);

private:
};

}  // namespace fase

#endif  // EDITOR_H_20180628
