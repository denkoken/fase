
#include "editor_gui_view.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "core_util.h"

namespace fase {

namespace {}  // namespace

Menu::~Menu() {}

View::View(const FaseCore& core, const TypeUtils& utils)
    : core(core), utils(utils) {
    setupMenus();
}

std::vector<Issue> View::draw(const std::string& win_title,
                              const std::string& label_suffix,
                              const std::map<std::string, Variable>& resp) {
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
            std::vector<Issue> issue_reqs = menu->draw(resp);

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
