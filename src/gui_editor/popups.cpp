
#include <imgui.h>
#include <imgui_internal.h>

#include "view.h"
#include "../core_util.h"

namespace fase {

namespace {

// Additional ImGui components
bool IsItemActivePreviousFrame() {
    ImGuiContext* g = ImGui::GetCurrentContext();
    if (g->ActiveIdPreviousFrame) {
        return g->ActiveIdPreviousFrame == GImGui->CurrentWindow->DC.LastItemId;
    }
    return false;
}

bool IsKeyPressed(ImGuiKey_ key, bool repeat = true) {
    return ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[key], repeat);
};

bool IsKeyPressedOnItem(ImGuiKey_ key, bool repeat = true) {
    return IsItemActivePreviousFrame() && IsKeyPressed(key, repeat);
};

bool Combo(const char* label, int* curr_idx, std::vector<std::string>& vals) {
    if (vals.empty()) {
        return false;
    }
    return ImGui::Combo(
            label, curr_idx,
            [](void* vec, int idx, const char** out_text) {
                auto& vector = *static_cast<std::vector<std::string>*>(vec);
                if (idx < 0 || static_cast<int>(vector.size()) <= idx) {
                    return false;
                } else {
                    *out_text = vector.at(size_t(idx)).c_str();
                    return true;
                }
            },
            static_cast<void*>(&vals), int(vals.size()));
}



class PopupContent : public Content {
public:
    template <class... Args>
    PopupContent(std::string&& popup_name, bool menu, Args&&... args)
        : Content(args...), popup_name(popup_name), menu(menu) {}

    virtual ~PopupContent() {};

protected:
    virtual void layout() = 0;

    virtual bool init() { return true; }

private:
    std::string popup_name;
    bool menu;

    void main() {
        if (exists(state.popup_issue, popup_name)) {
            state.popup_issue.erase(std::find(std::begin(state.popup_issue),
                                              std::end(state.popup_issue),
                                              popup_name));
            if (init()) {
                ImGui::OpenPopup(label(popup_name.c_str()));
            }
        }
        bool opened = true;
        ImGuiWindowFlags option = ImGuiWindowFlags_AlwaysAutoResize;
        if (menu) {
            option |= ImGuiWindowFlags_MenuBar;
        }
        if (ImGui::BeginPopupModal(label(popup_name.c_str()), &opened, option)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            layout();

            ImGui::Separator();

            if (ImGui::Button(label("Done"))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();

        }
    }
};

class NativeCodePopup : public PopupContent {
public:
    template <class... Args>
    NativeCodePopup(Args&&... args) : PopupContent(POPUP_NATIVE_CODE, false, args...) {}

    ~NativeCodePopup() {}
private:

    std::string native_code;

    bool init() {
        native_code = GenNativeCode(core, utils);
        return true;
    }

    void layout() {
        ImGui::InputTextMultiline(label("##native code"),
                                  const_cast<char*>(native_code.c_str()),
                                  native_code.size(), ImVec2(500, 500),
                                  ImGuiInputTextFlags_ReadOnly);
    }
};

class AddingNodePopup : public PopupContent {
public:
    template <class... Args>
    AddingNodePopup(Args&&... args) : PopupContent(POPUP_ADDING_NODE, false, args...) {}

    ~AddingNodePopup() {}
private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    std::vector<std::string> func_reprs;
    char name_buf[64] = "";
    int curr_idx = 0;
    std::string error_msg;

    void updateFuncReprs(const std::map<std::string, Function>& functions) {
        if (functions.size() != func_reprs.size()) {
            // Create all again
            func_reprs.clear();
            for (auto it = functions.begin(); it != functions.end(); it++) {
                if (IsSpecialFuncName(it->first)) {
                    continue;
                }
                func_reprs.push_back(it->first);
            }
        }
    }

    bool init() {
        error_msg.clear();
        curr_idx = 0;
        std::stringstream ss;
        ss << "node_" << core.getNodes().size() - 2;
        strncpy(name_buf, ss.str().c_str(), sizeof(name_buf));
        return true;
    }

    void layout() {

        // Extract function representations
        updateFuncReprs(core.getFunctions());

        // Input elements
        ImGui::InputText(label("Node name (ID)"), name_buf,
                         sizeof(name_buf));
        const bool enter_pushed = IsKeyPressedOnItem(ImGuiKey_Enter);
        Combo(label("Function"), &curr_idx, func_reprs);
        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }

        bool success;
        if (throwIssue(IssuePattern::AddNode,
                       ImGui::Button(label("OK")) || enter_pushed,
                       AddNodeInfo{name_buf, func_reprs[size_t(curr_idx)]},
                       &success)) {
            if (success) {
                ImGui::CloseCurrentPopup();
            } else {
                error_msg = "Failed to create a new node";  // Failed
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(label("Cancel")) ||
            IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }
    }
};

class InputOutputPopup : public PopupContent {
public:
    template <class... Args>
    InputOutputPopup(Args&&... args)
        : PopupContent(POPUP_INPUT_OUTPUT, false, args...) {}

    ~InputOutputPopup() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    char buf[64] = "";
    std::string error_msg;

    void layout() {
        ImGui::InputText(label("Name"), buf, sizeof(buf));

        if (!error_msg.empty()) {
            ImGui::TextColored(ImVec4(255.f, 0.f, 0.f, 255.f), "%s",
                               error_msg.c_str());
        }

        bool success;
        if (issueButton(IssuePattern::AddInput, std::string(buf), &success,
                        "Make input")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Invalid Name";  // Failed
            }
        }

        if (issueButton(IssuePattern::AddOutput, std::string(buf), &success,
                        "Make output")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Invalid Name";  // Failed
            }
        }

        if (core.getNodes().at(InputNodeStr()).links.size() > 0) {
            issueButton(
                    IssuePattern::DelInput,
                    size_t(core.getNodes().at(InputNodeStr()).links.size() -
                           1),
                    &success, "Delelte input");
#if 0
            // TODO
            ImGui::SameLine();
            if (ImGui::Button(label("Reset Input"))) {
                const size_t arg_n =
                        core->getNodes().at(InputNodeStr()).links.size();
                for (size_t i = arg_n - 1; i < arg_n; i--) {
                    core->delInput(arg_n);
                }
                updater();
                ImGui::CloseCurrentPopup();
            }
#endif
        }
        if (core.getNodes().at(OutputNodeStr()).links.size() > 0) {
            issueButton(IssuePattern::DelOutput,
                        size_t(core.getNodes()
                                       .at(OutputNodeStr())
                                       .links.size() -
                               1),
                        &success, "Delelte output");
#if 0
            // TODO
            ImGui::SameLine();
            if (ImGui::Button(label("Reset Output"))) {
                const size_t arg_n =
                        core->getNodes().at(OutputNodeStr()).links.size();
                for (size_t i = arg_n - 1; i < arg_n; i--) {
                    core->delOutput(arg_n);
                }
                updater();
                ImGui::CloseCurrentPopup();
            }
#endif
        }
    }
};

class ProjectPopup : public PopupContent {
public:
    template <class... Args>
    ProjectPopup(Args&&... args)
        : PopupContent(POPUP_PROJECT, false, args...) {}

    ~ProjectPopup() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);
    char load_filename_buf[128] = "fase_save.txt";
    char save_filename_buf[128];
    char project_name_buf[128] = "NewProject";
    int combo_idx = 0;
    std::string error_msg;

    enum struct Pattern {
        New,
        Save,
        Load,
        Rename,
        Switch,
    } pattern = Pattern::Save;

    bool init() {
        std::strcpy(save_filename_buf, (core.getProjectName() + ".txt").c_str());
        combo_idx = 0;
        return true;
    }

    void save() {
        ImGui::Text("Save Project");
        ImGui::Separator();

        ImGui::Text("File path :");
        ImGui::SameLine();
        ImGui::InputText(label(""), save_filename_buf,
                         sizeof(save_filename_buf));

        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }

        bool success;
        if (issueButton(IssuePattern::Save, std::string(save_filename_buf),
                        &success, "OK")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Failed to save pipeline";  // Failed
            }
        }
    }

    void load() {
        ImGui::Text("Load Project");
        ImGui::Separator();

        ImGui::Text("File path :");
        ImGui::SameLine();

        ImGui::InputText(label(""), load_filename_buf,
                         sizeof(load_filename_buf));

        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }

        bool success;
        if (issueButton(IssuePattern::Load, std::string(load_filename_buf),
                        &success, "OK")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Failed to load pipeline";  // Failed
            }
        }
    }

    void new_project() {
        ImGui::Text("New Project");
        ImGui::Separator();

        ImGui::Text("Project Name :");
        ImGui::SameLine();
        ImGui::InputText(label(""), project_name_buf,
                         sizeof(project_name_buf));

        bool success;
        if (issueButton(IssuePattern::SwitchProject,
                        std::string(project_name_buf), &success,
                        "New Project")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void rename() {
        ImGui::Text("Rename Project");
        ImGui::Separator();

        ImGui::Text("Project New Name :");
        ImGui::SameLine();
        // Input elements
        ImGui::InputText(label(""), project_name_buf,
                         sizeof(project_name_buf));

        bool success;
        if (issueButton(IssuePattern::RenameProject,
                        std::string(project_name_buf), &success,
                        "Rename")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void switch_project() {
        std::vector<std::string> projects = core.getProjects();
        Combo(label("project"), &combo_idx, projects);

        bool success;
        if (issueButton(IssuePattern::SwitchProject,
                        projects[size_t(combo_idx)], &success, "Switch")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void layout() {
        // ImGui::BeginMenuBar();
        ImGui::BeginChild(label("project left panel"), ImVec2(200, 400));
        ImGui::Text("");
        ImGui::Separator();
        if (ImGui::Selectable(label("New Project"), pattern == Pattern::New)) {
            pattern = Pattern::New;
        }
        if (ImGui::Selectable(label("Save Project"), pattern == Pattern::Save)) {
            pattern = Pattern::Save;
        }
        if (ImGui::Selectable(label("Load Project"), pattern == Pattern::Load)) {
            pattern = Pattern::Load;
        }
        if (ImGui::Selectable(label("Switch Project"), pattern == Pattern::Switch)) {
            pattern = Pattern::Switch;
        }
        if (ImGui::Selectable(label("Rename Project"), pattern == Pattern::Rename)) {
            pattern = Pattern::Rename;
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild(label("project right panel"), ImVec2(300, 400));
        if (pattern == Pattern::Save) {
            save();
        } else if (pattern == Pattern::Load) {
            load();
        } else if (pattern == Pattern::New) {
            new_project();
        } else if (pattern == Pattern::Switch) {
            switch_project();
        } else if (pattern == Pattern::Rename) {
            rename();
        }
        ImGui::EndChild();
    }
};

class PreferencePopup : public PopupContent {
public:
    template <class... Args>
    PreferencePopup(Args&&... args)
        : PopupContent(POPUP_PREFERENCE, false, args...) {}

    ~PreferencePopup() {}
private:
    void layout() {

        { // Node edit Settings
            constexpr int v_min = std::numeric_limits<int>::min();
            constexpr int v_max = std::numeric_limits<int>::max();
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("priority min"), &preference.priority_min,
                           v_speed, v_min, preference.priority_max);
            ImGui::DragInt(label("priority max"), &preference.priority_max,
                           v_speed, preference.priority_min, v_max);
        }

        ImGui::Separator();

        { // Node view Settings
            ImGui::Checkbox(label("Auto Layout Canvas"),
                            &preference.auto_layout);
            ImGui::Checkbox(label("Simple Node Boxes"),
                            &preference.is_simple_node_box);

            constexpr int v_min = 3;
            constexpr int v_max = 64;
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("argument char max"),
                           &preference.max_arg_name_chars, v_speed, v_min,
                           v_max);
        }

        ImGui::Separator();

        { // Panel Settings
            ImGui::Checkbox(label("Edit Panel View"),
                            &preference.enable_edit_panel);
            ImGui::Checkbox(label("Node List Panel View"),
                            &preference.enable_node_list_panel);
            constexpr int v_min = 0;
            constexpr int v_max = std::numeric_limits<int>::max();
            constexpr float v_speed = 0.5;

            ImGui::DragInt(label("node list panel size"),
                           &preference.node_list_panel_size, v_speed, v_min,
                           v_max);
            ImGui::DragInt(label("edit panel size"),
                           &preference.edit_panel_size, v_speed, v_min, v_max);
        }
    }

};

/// Dummy class. Don't use this without a calling SetupContents().
class Footer {};

template <class Head, class... Tail>
void SetupContents(const FaseCore& core, LabelWrapper& label, GUIState& state,
                const TypeUtils& utils, std::function<void(Issue)> issue_f,
                std::vector<std::unique_ptr<Content>>* contents) {
    contents->emplace_back(
            std::make_unique<Head>(core, label, state, utils, issue_f));
    SetupContents<Tail...>(core, label, state, utils, issue_f, contents);
}

template <>
void SetupContents<Footer>(const FaseCore&, LabelWrapper&, GUIState&,
                        const TypeUtils&, std::function<void(Issue)>,
                        std::vector<std::unique_ptr<Content>>*) {}

} // namespace

void View::setupPopups(std::function<void(Issue)>&& issue_f) {
    SetupContents<PreferencePopup, NativeCodePopup, AddingNodePopup, InputOutputPopup,
                  ProjectPopup, Footer>(
            core, label, state, utils, issue_f, &popups);
}

} // namespace fase
