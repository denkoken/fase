
#include "imgui_editor.h"

#include "imgui_commons.h"

namespace fase {

class ImGuiEditor::Impl {
public:
    void addOptinalButton(std::string&& name, std::function<void()>&& callback,
                          std::string&& description);

    bool runEditing(const std::string& win_title,
                    const std::string& label_suffix);

    bool init();

    bool addVarEditor(std::type_index&& type, VarEditorWraped&& f);

private:
};

void ImGuiEditor::Impl::addOptinalButton(std::string&& name,
                                         std::function<void()>&& callback,
                                         std::string&& description) {}

bool ImGuiEditor::Impl::runEditing(const std::string& win_title,
                                   const std::string& label_suffix) {
    bool p;
    if (auto raii = WindowRAII(win_title + label_suffix, &p)) {
        ImGui::Text("test");
    }
    return true;
}

bool ImGuiEditor::Impl::init() {
    return true;
}

bool ImGuiEditor::Impl::addVarEditor(std::type_index&& type,
                                     VarEditorWraped&& f) {
    return true;
}

// ============================== Pimpl Pattern ================================

ImGuiEditor::ImGuiEditor() : pimpl(std::make_unique<Impl>()) {}
ImGuiEditor::~ImGuiEditor() = default;

void ImGuiEditor::addOptinalButton(std::string&& name,
                                   std::function<void()>&& callback,
                                   std::string&& description) {
    pimpl->addOptinalButton(std::move(name), std::move(callback),
                            std::move(description));
}

bool ImGuiEditor::runEditing(const std::string& win_title,
                             const std::string& label_suffix) {
    return pimpl->runEditing(win_title, label_suffix);
}

bool ImGuiEditor::init() {
    return pimpl->init();
}

bool ImGuiEditor::addVarEditor(std::type_index&& type, VarEditorWraped&& f) {
    return pimpl->addVarEditor(std::move(type), std::move(f));
}

}  // namespace fase
