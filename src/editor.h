#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"
#include "type_utils.h"

#include "parts_base.h"

namespace fase {

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
template <typename T>
using VarEditor = std::function<std::unique_ptr<T>(const char*, const T&)>;
using VarEditorWraped = std::function<Variable(const char*, const Variable&)>;

namespace guieditor {

class GUIEditor : public PartsBase {
public:
    GUIEditor(const TypeUtils&);
    ~GUIEditor();

    /**
     * @brief
     *      add variable editor
     *
     * @tparam T
     *      type of variable
     * @param var_editor
     *      (const char*, const T&) -> std::unique_ptr<T>
     *      arguments :
     *      First argument is name of variable (the argument name of node).
     *      Second argument is the value of variable.
     *      return :
     *      If returned is empty ptr, it means no change of the variable.
     *      Otherwise, the variable repaced with pointed by returned.
     *
     * @return
     *      succeeded or not. maybe allways true will be returned.
     */
    template <typename T>
    bool addVarEditor(VarEditor<T>&& var_editor);

    /**
     * @brief
     *      buffer the imgui draw objects.
     *
     * @param win_title
     *      the title of editor imgui window.
     * @param label_suffix
     *      label string of imgui object.
     *
     * @return
     *      succeeded or not.
     */
    bool runEditing(const std::string& win_title = "Fase Editor",
                    const std::string& label_suffix = "##fase");

private:
    /**
     * @brief add wraped variable editor.
     *
     * @param p type_info of type of variable.
     * @param f wraped variable editor
     *
     * @return succeeded or not.
     */
    bool addVarEditor(const std::type_info* p, VarEditorWraped&& f);

    // pImpl pattern
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace guieditor

using guieditor::GUIEditor;

}  // namespace fase

#include "gui_editor/editor_impl.h"

#endif  // EDITOR_H_20180628
