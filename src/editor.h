#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"

namespace fase {

template <typename P>
class EditorBase {
public:
    virtual ~EditorBase() {}

    template <typename Gen>
    bool addVarGenerator(Gen &&gen) {
        return static_cast<P*>(this)->template addVarGenerator<Gen>(
                std::forward<Gen>(gen));
    }

    virtual void start(FaseCore*) {}
};

class CLIEditor : public EditorBase<CLIEditor> {
public:
    template <typename T>
    bool addVarGenerator(const std::function<T(const std::string&)> &func) {
        var_generators[&typeid(T)] = func;
        return true;
    }

    auto getVarGenerators() { return var_generators; }

    void start(FaseCore* core);

private:
    // Variable generators
    std::map<const std::type_info*, std::function<Variable(const std::string&)>>
            var_generators;
};

}  // namespace fase

#endif  // EDITOR_H_20180628
