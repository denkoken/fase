
#include "editor_gui_view.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace fase {

namespace {

class PreferenceMenu : public Menu {
public:
    PreferenceMenu(LabelWrapper& label, GUIPreference& preference)
        : Menu(label, preference) {}

    ~PreferenceMenu() {}

    std::vector<Issue> draw(const FaseCore&) {
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

#if 0
// Draw button and pop up window for native code
class NativeCodeGUI : public Menu {
public:
    NativeCodeGUI(LabelWrapper& label, GUIPreference& preference)
        : Menu(label, preference) {} {}

    std::vector<Issue> draw(const FaseCore&) {
        if (ImGui::MenuItem(label("Show code"))) {
            // Update all arguments
            // updateAllArgs(core);
            // Generate native code
            native_code = GenNativeCode(*core, utils);
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
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Reference to the parent's
    LabelWrapper& label;
    std::map<const std::type_info*, VarEditor> var_editors;

    // Private status
    std::string native_code;
};
#endif

/// Dummy class. Don't use this without a calling SetupMenus().
class Footer {};

template <class Head, class... Tail>
void SetupMenus(LabelWrapper& label, GUIPreference& preference,std::vector<std::unique_ptr<Menu>>* menus) {
    menus->emplace_back(std::make_unique<Head>(label, preference));
    SetupMenus<Tail...>(label, preference, menus);
}

template <>
void SetupMenus<Footer>(LabelWrapper&, GUIPreference&,
                        std::vector<std::unique_ptr<Menu>>*) {}

// template <class... MenuClass>
// void SetupMenus(LabelWrapper& label, GUIPreference& preference,
//                 std::vector<std::unique_ptr<Menu>>* menus) {
//     *menus = { std::make_unique<MenuClass>(label, preference)... };
// }

}  // namespace

Menu::~Menu() {}

View::View() {
    // Setup Menu bar
    SetupMenus<PreferenceMenu, Footer>(label, preference, &menus);
}

std::vector<Issue> View::draw(const FaseCore& core,
                              const std::string& win_title,
                              const std::string& label_suffix) {
    std::vector<Issue> issues;

    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(win_title.c_str(), NULL, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return {};
    }

    label.setSuffix(label_suffix);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        for (std::unique_ptr<Menu>& menu : menus) {
            // draw menu.
            std::vector<Issue> issue_reqs = menu->draw(core);

            // add issues.
            for (Issue& req : issue_reqs) {
                issues.emplace_back(std::move(req));
            }
        }
        ImGui::EndMenuBar();
    }

    // TODO

    ImGui::End();  // End window

    return issues;
}

}  // namespace fase
