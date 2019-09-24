
#ifndef IMGUI_EDITOR_H_20190223
#define IMGUI_EDITOR_H_20190223

#include "../parts_base.h"

namespace fase {

template <typename T>
using VarEditor = std::function<std::unique_ptr<T>(const char*, const T&)>;
using VarEditorWraped = std::function<Variable(const char*, const Variable&)>;

// clang-format off
/**
 * @brief
 *      You can use the gui editor using imgui library, with this part class.  
 *      To know how to use, see `examples/guieditor/fase_gl_utils.h` and
 *      `examples/guieditor/main.cpp`.
 */
// clang-format on
class ImGuiEditor : public PartsBase {
public:
    ImGuiEditor();
    ImGuiEditor(const ImGuiEditor&);
    ImGuiEditor(ImGuiEditor&);
    ImGuiEditor(ImGuiEditor&&);
    ImGuiEditor& operator=(const ImGuiEditor&);
    ImGuiEditor& operator=(ImGuiEditor&);
    ImGuiEditor& operator=(ImGuiEditor&&);
    ~ImGuiEditor();

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
     *      Otherwise, the variable replaced with pointed by returned.
     *
     * @return
     *      succeeded or not. maybe allways true will be returned.
     */
    template <typename T>
    bool addVarEditor(VarEditor<T>&& var_editor);

    void addOptinalButton(std::string&& name, std::function<void()>&& callback,
                          std::string&& description = "");

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

protected:
    bool init();

private:
    /**
     * @brief add wraped variable editor.
     *
     * @param type typeid of type of variable.
     * @param f wraped variable editor
     *
     * @return succeeded or not.
     */
    bool addVarEditor(std::type_index&& type, VarEditorWraped&& f);

    // pImpl pattern
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

template <typename T>
bool ImGuiEditor::addVarEditor(VarEditor<T>&& var_editor) {
    return addVarEditor(typeid(T),
                        [var_editor](auto c, const Variable& var) -> Variable {
                            return var_editor(c, *var.getReader<T>());
                        });
}

} // namespace fase

#endif // IMGUI_EDITOR_H_20190223
