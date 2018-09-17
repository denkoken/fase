#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"
#include "type_utils.h"

namespace fase {

template <typename Parent>
class EditorBase {
public:
    ///
    /// Add variable generator
    ///  @return Success or not
    ///
    template <typename Gen>
    bool addVarGenerator(Gen &&) {}

    ///
    /// Run editor one step
    ///  @return Flag to continue the editing
    ///
    template <typename... Args>
    bool run(FaseCore *, Args...) {}
};

#if 0
// ------------------------------------ CLI ------------------------------------
class CLIEditor : public EditorBase<CLIEditor> {
public:
    CLIEditor() {}

    ///
    /// Add variable generator for a type "T"
    ///  It requires a function which convert "string" to "T".
    ///
    template <typename T>
    bool addVarGenerator(const std::function<T(const std::string &)> &func) {
        if (var_generators.count(&typeid(T))) {
            return false;
        } else {
            var_generators[&typeid(T)] = func;
            return true;
        }
    }

    bool run();

    auto getVarGenerators() {
        return var_generators;
    }

private:
    // Variable generators
    std::map<const std::type_info *,
             std::function<Variable(const std::string &)>>
            var_generators;
};
#endif

// ------------------------------------ GUI ------------------------------------
using GuiGeneratorFunc =
        std::function<bool(const char *, const Variable &, std::string &)>;

using VarEditor = std::function<Variable(const char *, const Variable &)>;

class GUIEditor : public EditorBase<GUIEditor> {
public:
    GUIEditor(FaseCore *, const TypeUtils &);
    ~GUIEditor();

    ///
    /// Add variable generator for a type "T"
    ///  It requires a function for drawing GUI.
    ///
    template <typename T>
    bool addVarGenerator(
            T, const std::function<bool(const char *, const Variable &,
                                        std::string &)> &func) {
        if (var_generators.count(&typeid(T))) {
            return false;
        } else {
            var_generators[&typeid(T)] = func;
            return true;
        }
    }

    bool addVarEditor(const std::type_info *p, VarEditor &&f);

    bool run(const std::string &win_title = "Fase Editor",
             const std::string &label_suffix = "##fase");

private:
    // Variable generators
    std::map<const std::type_info *, GuiGeneratorFunc> var_generators;

    // pImpl pattern
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace fase

#endif  // EDITOR_H_20180628
