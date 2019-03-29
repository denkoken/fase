
#include "imgui_editor.h"

#include <cmath>
#include <deque>

#include "../utils.h"
#include "imgui_commons.h"
#include "pipe_edit_window.h"
#include "setup_var_editors.h"

namespace fase {

using std::map, std::string, std::vector;
using namespace std::literals::chrono_literals;

namespace {

struct OptionalButton {
    std::string name;
    std::function<void()> callback;
    std::string description;
};

void DrawOptionalButtons(vector<OptionalButton>& optional_buttons,
                         LabelWrapper label) {
    label.addSuffix("##OptionalButton");
    for (auto& ob_data : optional_buttons) {
        if (ImGui::Button(label(ob_data.name))) {
            ob_data.callback();
        }
        ImGui::SameLine();
        ImGui::Text("%s", ob_data.description.c_str());
    }
}

void SetGuiStyle() {
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.GrabRounding = style.FrameRounding = 5.0;
    style.FrameBorderSize = 0.3f;
}

}  // namespace

class ImGuiEditor::Impl {
public:
    Impl(ImGuiEditor* parent) : pparent(parent) {}

    void addOptinalButton(std::string&& name, std::function<void()>&& callback,
                          std::string&& description);

    bool runEditing(const std::string& win_title,
                    const std::string& label_suffix);

    bool init();

    bool addVarEditor(std::type_index&& type, VarEditorWraped&& f);

private:
    vector<string> opened_pipelines;
    ImGuiEditor* const pparent;

    Issues issues;

    VarEditors var_editors;

    // Control Window's variables
    vector<OptionalButton> optional_buttons;
    InputText new_core_name_it{64, ImGuiInputTextFlags_EnterReturnsTrue};
    Combo pipeline_names_combo;
    // for save pipeline
    InputText filename_it{256, ImGuiInputTextFlags_EnterReturnsTrue |
                                       ImGuiInputTextFlags_AutoSelectAll};
    // for store native code
    std::string native_code;

    Combo focused_pipeline_names_combo;

    // Pipeline Edit Window
    map<string, EditWindow> edit_windows;

    bool drawEditWindows(const string& win_title, LabelWrapper label);

    bool drawControlWindows(const string& win_title, LabelWrapper label);

    bool doIssues();
};

void ImGuiEditor::Impl::addOptinalButton(std::string&& name,
                                         std::function<void()>&& callback,
                                         std::string&& description) {
    optional_buttons.emplace_back(OptionalButton{
            std::move(name), std::move(callback), std::move(description)});
}

bool ImGuiEditor::Impl::drawEditWindows(const string& win_title,
                                        LabelWrapper label) {
    auto [lock, pcm] = pparent->getReader(16ms);
    if (!lock) {
        return true;
    }

    vector<string> should_be_closeds;
    for (auto& p_name : opened_pipelines) {
        bool opened = edit_windows[p_name].draw(p_name, win_title, *pcm, label,
                                                &issues, &var_editors);
        if (!opened) {
            should_be_closeds.emplace_back(p_name);
        }
    }
    for (auto& be_closed : should_be_closeds) {
        erase_all(opened_pipelines, be_closed);
        edit_windows.erase(be_closed);
    }
    return true;
}

bool ImGuiEditor::Impl::drawControlWindows(const string& win_title,
                                           LabelWrapper label) {
    label.addSuffix("##ControlWindow");
    if (auto raii = WindowRAII(label(win_title + " : Control Window"), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        auto [lock, pcm] = pparent->getReader(16ms);
        if (!lock) {
            return false;
        }
        ImGui::Text("Make a new pipeline : ");
        if (new_core_name_it.draw(label("##core_open_name"))) {
            if (!exists(new_core_name_it.text(), opened_pipelines)) {
                issues.emplace_back(
                        [name = new_core_name_it.text(), this](auto pcm) {
                            if (!(*pcm)[name].getNodes().empty()) {
                                opened_pipelines.emplace_back(name);
                            }
                        });
                new_core_name_it.set("");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool load_f = ImGui::Button(label("Load Pipeline..."));
        if (auto p_raii = BeginPopupModal(label("load popup"), load_f)) {
            filename_it.draw(label("filename"));
            if (ImGui::Button(label("OK"))) {
                issues.emplace_back(
                        [filename = filename_it.text(), this](auto pcm) {
                            LoadPipelineFromFile(filename, pcm,
                                                 pparent->getConverterMap());
                        });
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        pipeline_names_combo.set(pcm->getPipelineNames());
        pipeline_names_combo.draw(label("pipeline"));
        if (auto p_name = pipeline_names_combo.text(); !p_name.empty()) {
            bool save_f = ImGui::Button(label("Save..."));
            ImGui::SameLine();
            if (ImGui::Button(label("Open")) &&
                !exists(p_name, opened_pipelines)) {
                opened_pipelines.emplace_back(p_name);
            }
            if (save_f) {
                filename_it.set(p_name + ".txt");
            }
            if (auto p_raii = BeginPopupModal(label("save popup"), save_f)) {
                filename_it.draw(label("filename"));
                if (ImGui::Button(label("OK"))) {
                    SavePipeline(p_name, *pcm, filename_it.text(),
                                 pparent->getConverterMap());
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            // show native code.
            bool code_f = ImGui::Button(label("Show code..."));
            if (code_f) {
                native_code = GenNativeCode(p_name, *pcm,
                                            pparent->getConverterMap(), p_name);
            }
            if (auto p_raii =
                        BeginPopupModal(label("Native Code"), code_f, true)) {
                ImGui::InputTextMultiline(label("code"), &native_code[0],
                                          native_code.size(), ImVec2(800, 600),
                                          ImGuiInputTextFlags_ReadOnly);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        focused_pipeline_names_combo.set(pcm->getPipelineNames());
        focused_pipeline_names_combo.draw(label("focused pipeline"));
        if (auto p_name = focused_pipeline_names_combo.text();
            !p_name.empty()) {
            issues.emplace_back(
                    [p_name](auto pcm) { pcm->setFocusedPipeline(p_name); });
        }

        release(lock, pcm);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        DrawOptionalButtons(optional_buttons, label);
    }
    return true;
}

bool ImGuiEditor::Impl::doIssues() {
    if (issues.empty()) return true;
    auto [lock, pcm] = pparent->getWriter(16ms);
    if (!pcm) return false;
    for (auto issue : issues) {
        issue(pcm.get());
    }
    issues.clear();
    return true;
}

bool ImGuiEditor::Impl::runEditing(const std::string& win_title,
                                   const std::string& label_suffix) {
    LabelWrapper label(label_suffix);
    SetGuiStyle();

    drawControlWindows(win_title, label);
    drawEditWindows(win_title, label);

    doIssues();

    return true;
}

bool ImGuiEditor::Impl::init() {
    SetupDefaultVarEditors(&var_editors);
    return true;
}

bool ImGuiEditor::Impl::addVarEditor(std::type_index&& type,
                                     VarEditorWraped&& f) {
    var_editors.erase(type);
    var_editors.emplace(type, f);
    return true;
}

// ============================== Pimpl Pattern ================================

ImGuiEditor::ImGuiEditor() : pimpl(std::make_unique<Impl>(this)) {}
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
