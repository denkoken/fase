
#include "editor_gui_view.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "core_util.h"

namespace fase {

namespace {

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
            if (!opened || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
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
            if (!opened || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
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
    SetupMenus<PreferenceMenu, NativeCodeMenu, SaveMenu, Footer>(
            core, label, preference, utils, &menus);
}

}  // namespace fase
