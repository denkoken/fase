
#include "view.h"

#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

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

template <class Layout>
inline void Popup(const char* popup_name, const Layout& layout) {
    bool opened = true;
    if (ImGui::BeginPopupModal(popup_name, &opened,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();  // Behavior of close button
        }

        layout();

        if (ImGui::Button("Done")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

class PreferenceMenu : public Content {
public:
    template <class... Args>
    PreferenceMenu(Args&&... args) : Content(args...) {}

    ~PreferenceMenu() {}

private:
    void priorityPopUp(const char* popup_name) {
        constexpr int v_min = std::numeric_limits<int>::min();
        constexpr int v_max = std::numeric_limits<int>::max();
        constexpr float v_speed = 0.5;
        Popup(label(popup_name), [&]() {
            ImGui::DragInt(label("priority min"), &preference.priority_min,
                           v_speed, v_min, preference.priority_max);
            ImGui::DragInt(label("priority max"), &preference.priority_max,
                           v_speed, preference.priority_min, v_max);
        });
    }

    void nodeViewPopUp(const char* popup_name) {
        Popup(label(popup_name), [&]() {
            ImGui::Checkbox(label("Simple Node Boxes"),
                            &preference.is_simple_node_box);

            constexpr int v_min = 3;
            constexpr int v_max = 64;
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("argument char max"),
                           &preference.max_arg_name_chars, v_speed, v_min,
                           v_max);
        });
    }

    void panelPopUp(const char* popup_name) {
        Popup(label(popup_name), [&]() {
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
        });
    }

    void main() {
        bool priority_f = false;
        bool node_f = false;
        bool panel_f = false;
        if (ImGui::BeginMenu("Preferences")) {
            ImGui::MenuItem(label("Auto Layout Sorting"), NULL,
                            &preference.auto_layout);
            if (ImGui::MenuItem(label("Node View Settings.."), NULL)) {
                node_f = true;
            }
            if (ImGui::MenuItem(label("Priority Settings.."), NULL)) {
                priority_f = true;
            }
            if (ImGui::MenuItem(label("Panel Settings.."), NULL)) {
                panel_f = true;
            }
            ImGui::EndMenu();
        }
        if (node_f) {
            ImGui::OpenPopup(label("Popup: Node View setting"));
        }
        if (priority_f) {
            ImGui::OpenPopup(label("Popup: Priority setting"));
        }
        if (panel_f) {
            ImGui::OpenPopup(label("Popup: Panel setting"));
        }
        priorityPopUp("Popup: Priority setting");
        nodeViewPopUp("Popup: Node View setting");
        panelPopUp("Popup: Panel setting");
    }
};

// Draw button and pop up window for native code
class NativeCodeMenu : public Content {
public:
    template <class... Args>
    NativeCodeMenu(Args&&... args) : Content(args...) {}

    ~NativeCodeMenu() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Private status
    std::string native_code;

    void main() {
        if (ImGui::MenuItem(label("Show code"))) {
            // Generate native code
            native_code = GenNativeCode(core, utils);
            // Open pop up window
            ImGui::OpenPopup(label("Popup: Native code"));
        }
        Popup(label("Popup: Native code"), [&]() {
            ImGui::InputTextMultiline(label("##native code"),
                                      const_cast<char*>(native_code.c_str()),
                                      native_code.size(), ImVec2(500, 500),
                                      ImGuiInputTextFlags_ReadOnly);
        });
    }
};

class ProjectMenu : public Content {
public:
    template <class... Args>
    ProjectMenu(Args&&... args) : Content(args...) {}

    ~ProjectMenu() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);
    char filename_buf[128] = "fase_save.txt";
    char project_name_buf[128] = "NewProject";
    std::string error_msg;
    int combo_idx = 0;

    void savePopup(const char* popup_name) {
        Popup(popup_name, [&]() {
            // Input elements
            ImGui::InputText(label("File path"), filename_buf,
                             sizeof(filename_buf));

            if (!error_msg.empty()) {
                ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
            }

            bool success;
            if (issueButton(IssuePattern::Save, std::string(filename_buf),
                            &success, "OK")) {
                if (success) {
                    ImGui::CloseCurrentPopup();
                    error_msg = "";
                } else {
                    error_msg = "Failed to save pipeline";  // Failed
                }
            }
        });
    }

    void loadPopup(const char* popup_name) {
        Popup(popup_name, [&]() {
            // Input elements
            ImGui::InputText(label("File path"), filename_buf,
                             sizeof(filename_buf));

            if (!error_msg.empty()) {
                ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
            }

            bool success;
            if (issueButton(IssuePattern::Load, std::string(filename_buf),
                            &success, "OK")) {
                if (success) {
                    ImGui::CloseCurrentPopup();
                    error_msg = "";
                } else {
                    error_msg = "Failed to load pipeline";  // Failed
                }
            }
        });
    }

    void switchPopup(const char* popup_name) {
        Popup(popup_name, [&]() {
            std::vector<std::string> projects = core.getProjects();
            Combo(label("issue"), &combo_idx, projects);

            bool success;
            if (issueButton(IssuePattern::SwitchProject,
                            projects[size_t(combo_idx)], &success, "Switch")) {
                ImGui::CloseCurrentPopup();
            }
        });
    }

    void newPopup(const char* popup_name) {
        Popup(popup_name, [&]() {
            // Input elements
            ImGui::InputText(label("File path"), project_name_buf,
                             sizeof(project_name_buf));

            bool success;
            if (issueButton(IssuePattern::SwitchProject,
                            std::string(project_name_buf), &success,
                            "New Project")) {
                ImGui::CloseCurrentPopup();
            }
        });
    }

    void renamePopup(const char* popup_name) {
        Popup(popup_name, [&]() {
            // Input elements
            ImGui::InputText(label("File path"), project_name_buf,
                             sizeof(project_name_buf));

            bool success;
            if (issueButton(IssuePattern::RenameProject,
                            std::string(project_name_buf), &success,
                            "Rename")) {
                ImGui::CloseCurrentPopup();
            }
        });
    }

    void main() {
        bool save_f = false, load_f = false, switch_f = false, new_f = false,
             rename_f = false;
        if (ImGui::BeginMenu("Project..")) {
            ImGui::MenuItem(label("New.."), NULL, &new_f);
            ImGui::Separator();
            ImGui::MenuItem(label("Save.."), NULL, &save_f);
            ImGui::MenuItem(label("Load.."), NULL, &load_f);
            ImGui::Separator();
            ImGui::MenuItem(label("Switch.."), NULL, &switch_f);
            ImGui::MenuItem(label("Rename.."), NULL, &rename_f);
            ImGui::EndMenu();
        }
        if (save_f) {
            ImGui::OpenPopup(label("Popup: Save project"));
            std::strcpy(filename_buf, (core.getProjectName() + ".txt").c_str());
        }
        if (load_f) {
            ImGui::OpenPopup(label("Popup: Load project"));
        }
        if (switch_f) {
            ImGui::OpenPopup(label("Popup: Switch project"));
        }
        if (new_f) {
            ImGui::OpenPopup(label("Popup: New project"));
            std::strcpy(project_name_buf, "NewProject");
        }
        if (rename_f) {
            ImGui::OpenPopup(label("Popup: Rename project"));
            std::strcpy(project_name_buf, "NewNameProject");
        }
        savePopup(label("Popup: Save project"));
        loadPopup(label("Popup: Load project"));
        switchPopup(label("Popup: Switch project"));
        newPopup(label("Popup: New project"));
        renamePopup(label("Popup: Rename project"));
    }
};

constexpr char RUNNING_RESPONSE_ID[] = "RUNNING_RESPONSE_ID";

class RunPipelineMenu : public Content {
public:
    template <class... Args>
    RunPipelineMenu(Args&&... args) : Content(args...) {}

private:
    // Private status
    bool multi = false;
    bool report = true;

    bool getRunning() {
        bool f = false;
        bool ret = getResponse(RUNNING_RESPONSE_ID, &f);
        return ret && f;
    }

    void main() {
        bool is_running = getRunning();
        if (!is_running && ImGui::BeginMenu("Run")) {
            // Run once
            if (ImGui::MenuItem(label("Run")) && !is_running) {
                throwIssue(RUNNING_RESPONSE_ID, IssuePattern::BuildAndRun,
                           BuildAndRunInfo{multi, preference.another_th_run});
            }
            ImGui::Dummy(ImVec2(5, 0));  // Spacing
            // Run by loop
            if (!is_running) {
                if (ImGui::MenuItem(label("Run (loop)"))) {
                    // Start loop
                    throwIssue(
                            RUNNING_RESPONSE_ID, IssuePattern::BuildAndRunLoop,
                            BuildAndRunInfo{multi, preference.another_th_run});
                }
            }
            ImGui::MenuItem(label("Multi Build"), NULL, &multi);
            ImGui::MenuItem(label("Reporting"), NULL, &report);
            ImGui::MenuItem(label("Running on another thread"), NULL,
                            &preference.another_th_run);

            ImGui::EndMenu();
        } else if (is_running) {
            if (ImGui::MenuItem(label("Stop!"))) {
                // Stop
                throwIssue(RUNNING_RESPONSE_ID, IssuePattern::StopRunLoop, {});
            }
        }
    }
};

// Draw button and pop up window for node adding
class NodeAddingMenu : public Content {
public:
    template <class... Args>
    NodeAddingMenu(Args&&... args) : Content(args...) {}

    ~NodeAddingMenu() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);
    const std::string resp_id = "AddNode";

    // Private status
    std::vector<std::string> func_reprs;
    char name_buf[1024] = "";
    int curr_idx = 0;
    std::string error_msg;

    // Private methods
    void setDefaultNodeName(const FaseCore& core) {
        std::stringstream ss;
        ss << "node_" << core.getNodes().size();
        strncpy(name_buf, ss.str().c_str(), sizeof(name_buf));
    }

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

    void closePopup() {
        name_buf[0] = '\0';
        curr_idx = 0;
        error_msg.clear();
        ImGui::CloseCurrentPopup();
    }

    void main() {
        if (ImGui::MenuItem(label("Add node"))) {
            // Set default name
            setDefaultNodeName(core);
            // Open pop up wijndow
            ImGui::OpenPopup(label("Popup: Add node"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: Add node"), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened) {
                closePopup();  // Behavior of close button
            }

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
                    closePopup();  // Success
                } else {
                    error_msg = "Failed to create a new node";  // Failed
                }
            }

            ImGui::SameLine();
            if (ImGui::Button(label("Cancel")) ||
                IsKeyPressed(ImGuiKey_Escape)) {
                closePopup();
            }

            ImGui::EndPopup();
        }
    }
};

// Draw button and pop up window for native code
class AddInOutputMenu : public Content {
public:
    template <class... Args>
    AddInOutputMenu(Args&&... args) : Content(args...) {}

    ~AddInOutputMenu() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    char buf[64] = "";
    std::string error_msg;

    void main() {
        if (ImGui::MenuItem(label("Add input/output"))) {
            // Open pop up window
            ImGui::OpenPopup(label("Popup: AddInOutputGUI"));
        }
        Popup(label("Popup: AddInOutputGUI"), [&]() {
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
        });
    }
};

class LayoutOptimizeMenu : public Content {
public:
    template <class... Args>
    LayoutOptimizeMenu(Args&&... args) : Content(args...) {}

    void main() {
        if (f) {
            preference.auto_layout = false;
            f = false;
        }
        if (ImGui::MenuItem(label("Optimize layout"))) {
            f = !preference.auto_layout;
            preference.auto_layout = true;
        }
    }

private:
    bool f = false;
};

/// Dummy class. Don't use this without a calling SetupMenus().
class Footer {};

template <class Head, class... Tail>
void SetupMenus(const FaseCore& core, LabelWrapper& label, GUIState& state,
                const TypeUtils& utils, std::function<void(Issue)> issue_f,
                std::vector<std::unique_ptr<Content>>* menus) {
    menus->emplace_back(
            std::make_unique<Head>(core, label, state, utils, issue_f));
    SetupMenus<Tail...>(core, label, state, utils, issue_f, menus);
}

template <>
void SetupMenus<Footer>(const FaseCore&, LabelWrapper&, GUIState&,
                        const TypeUtils&, std::function<void(Issue)>,
                        std::vector<std::unique_ptr<Content>>*) {}

// template <class... ContentClass>
// void SetupMenus(LabelWrapper& label, GUIPreference& preference,
//                 std::vector<std::unique_ptr<Content>>* menus) {
//     *menus = { std::make_unique<MenuClass>(label, preference)... };
// }

}  // namespace

int Content::id_counter = 0;

void View::setupMenus(std::function<void(Issue)>&& issue_f) {
    // Setup Menu bar
    SetupMenus<PreferenceMenu, ProjectMenu, NativeCodeMenu, NodeAddingMenu,
               RunPipelineMenu, AddInOutputMenu, LayoutOptimizeMenu, Footer>(
            core, label, state, utils, issue_f, &menus);
}

}  // namespace fase
