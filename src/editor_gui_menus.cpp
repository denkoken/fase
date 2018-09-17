
#include "editor_gui_view.h"

#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "core_util.h"

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

class PreferenceMenu : public Menu {
public:
    PreferenceMenu(const FaseCore& core, LabelWrapper& label,
                   GUIPreference& preference, const TypeUtils& utils)
        : Menu(core, label, preference, utils) {}

    ~PreferenceMenu() {}

    std::vector<Issue> draw(const std::map<std::string, Variable>&) {
        bool priority_f = false;
        if (ImGui::BeginMenu("Preferences")) {
            if (ImGui::MenuItem(label("Auto Layout Sorting"), NULL,
                                preference.auto_layout)) {
                preference.auto_layout = !preference.auto_layout;
            }
            if (ImGui::MenuItem(label("Priority Settings"), NULL)) {
                priority_f = true;
            }
            ImGui::EndMenu();
        }
        if (priority_f) {
            ImGui::OpenPopup(label("Popup: Priority setting"));
        }
        priorityPopUp("Popup: Priority setting");
        return {};
    }

private:
    void priorityPopUp(const char* popup_name) {
        bool opened = true;
        if (ImGui::BeginPopupModal(label(popup_name), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            constexpr int v_min = std::numeric_limits<int>::min();
            constexpr int v_max = std::numeric_limits<int>::max();
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("priority min"), &preference.priority_min,
                           v_speed, v_min, preference.priority_max);
            ImGui::DragInt(label("priority max"), &preference.priority_max,
                           v_speed, preference.priority_min, v_max);

            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
};

// Draw button and pop up window for native code
class NativeCodeMenu : public Menu {
public:
    NativeCodeMenu(const FaseCore& core, LabelWrapper& label,
                   GUIPreference& preference, const TypeUtils& utils)
        : Menu(core, label, preference, utils) {}

    ~NativeCodeMenu() {}

    std::vector<Issue> draw(const std::map<std::string, Variable>&) {
        if (ImGui::MenuItem(label("Show code"))) {
            // Generate native code
            native_code = GenNativeCode(core, utils);
            // Open pop up window
            ImGui::OpenPopup(label("Popup: Native code"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: Native code"), &opened,
                                   ImGuiWindowFlags_NoResize)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }
            ImGui::InputTextMultiline(label("##native code"),
                                      const_cast<char*>(native_code.c_str()),
                                      native_code.size(), ImVec2(500, 500),
                                      ImGuiInputTextFlags_ReadOnly);

            ImGui::EndPopup();
        }
        return {};
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Private status
    std::string native_code;
};

// FaseCore saver
class SaveMenu : public Menu {
public:
    SaveMenu(const FaseCore& core, LabelWrapper& label,
             GUIPreference& preference, const TypeUtils& utils)
        : Menu(core, label, preference, utils) {}

    ~SaveMenu() {}

    std::vector<Issue> draw(const std::map<std::string, Variable>& resp) {
        std::vector<Issue> dst;
        if (ImGui::MenuItem(label("Save"))) {
            ImGui::OpenPopup(label("Popup: Save pipeline"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: Save pipeline"), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened) {
                ImGui::CloseCurrentPopup();
            }

            // Input elements
            ImGui::InputText(label("File path"), filename_buf,
                             sizeof(filename_buf));

            if (!error_msg.empty()) {
                ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
            }

            if (ImGui::Button(label("OK"))) {
                Issue issue = {
                        .id = resp_id,
                        .issue = IssuePattern::Save,
                        .var = std::string(filename_buf),
                };
                dst.emplace_back(std::move(issue));
            }

            if (exists(resp, resp_id)) {
                if (*resp.at(resp_id).getReader<bool>()) {
                    ImGui::CloseCurrentPopup();
                    error_msg = "";
                } else {
                    error_msg = "Failed to save pipeline";  // Failed
                }
            }

            ImGui::EndPopup();
        }

        return dst;
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    const std::string resp_id = "SaveMenu";

    std::string error_msg;
    char filename_buf[1024] = "fase_save.txt";
};

// Draw button and pop up window for node adding
class NodeAddingMenu : public Menu {
public:
    // NodeAddingGUI(LabelWrapper& label, bool& request_add_node)
    //     : label(label), request_add_node(request_add_node) {}
    NodeAddingMenu(const FaseCore& core, LabelWrapper& label,
             GUIPreference& preference, const TypeUtils& utils)
        : Menu(core, label, preference, utils) {}

    ~NodeAddingMenu() {}

    std::vector<Issue> draw(const std::map<std::string, Variable>& resp) {
        std::vector<Issue> dst;
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
            if (ImGui::Button(label("OK")) || enter_pushed) {
                // Throw issue for Creating new node.
                Issue issue = {
                    .id = resp_id,
                    .issue = IssuePattern::AddNode,
                    .var = AddNodeInfo{ name_buf, func_reprs[size_t(curr_idx)] },
                };
                dst.emplace_back(issue);
            }
            ImGui::SameLine();
            if (ImGui::Button(label("Cancel")) ||
                IsKeyPressed(ImGuiKey_Escape)) {
                closePopup();
            }

            if (exists(resp, resp_id)) {
                if (*resp.at(resp_id).getReader<bool>()) {
                    closePopup();  // Success
                } else {
                    error_msg = "Failed to create a new node";  // Failed
                }
            }

            ImGui::EndPopup();
        }

        return dst;
    }

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
};

/// Dummy class. Don't use this without a calling SetupMenus().
class Footer {};

template <class Head, class... Tail>
void SetupMenus(const FaseCore& core, LabelWrapper& label,
                GUIPreference& preference, const TypeUtils& utils,
                std::vector<std::unique_ptr<Menu>>* menus) {
    menus->emplace_back(std::make_unique<Head>(core, label, preference, utils));
    SetupMenus<Tail...>(core, label, preference, utils, menus);
}

template <>
void SetupMenus<Footer>(const FaseCore&, LabelWrapper&, GUIPreference&,
                        const TypeUtils&, std::vector<std::unique_ptr<Menu>>*) {
}

// template <class... MenuClass>
// void SetupMenus(LabelWrapper& label, GUIPreference& preference,
//                 std::vector<std::unique_ptr<Menu>>* menus) {
//     *menus = { std::make_unique<MenuClass>(label, preference)... };
// }

}  // namespace

void View::setupMenus() {
    // Setup Menu bar
    SetupMenus<PreferenceMenu, NativeCodeMenu, NodeAddingMenu, SaveMenu, Footer>(
            core, label, preference, utils, &menus);
}

}  // namespace fase
