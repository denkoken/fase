
#include "editor_gui_view.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "core_util.h"

namespace fase {

namespace {

// Visible GUI node
struct GuiNode {
    ImVec2 pos;
    ImVec2 size;
    std::vector<ImVec2> arg_poses;
    std::vector<char> arg_inp_hovered;
    std::vector<char> arg_out_hovered;

    size_t arg_size() const {
        return arg_poses.size();
    }

    void alloc(size_t n_args) {
        arg_poses.resize(n_args);
        arg_inp_hovered.resize(n_args);
        arg_out_hovered.resize(n_args);
    }

    ImVec2 getInputSlot(const size_t idx) const {
        return arg_poses[idx];
    }
    ImVec2 getOutputSlot(const size_t idx) const {
        return ImVec2(arg_poses[idx].x + size.x, arg_poses[idx].y);
    }
};

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

}  // namespace

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
#if 0
    // Left side: Panel
    ImGui::BeginChild(label("left panel"), ImVec2(150, 0));
    {
        // Draw a list of nodes on the left side
        node_list_gui.draw(core);
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Right side: Canvas
    ImGui::BeginChild(label("right canvas"));
    {
        ImGui::Text("Hold middle mouse button to scroll (%f, %f)", scroll_pos.x,
                    scroll_pos.y);
        BeginCanvas(label("scrolling_region"));
        {
            // Draw grid canvas
            DrawCanvas(scroll_pos, 64.f);
            // Draw links
            links_gui.draw(core, updater);
            // Draw nodes
            node_boxes_gui.draw(core, preference);

            // Canvas scroll
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
                ImGui::IsMouseDragging(2, 0.f)) {
                scroll_pos = scroll_pos + ImGui::GetIO().MouseDelta;
            }
            // Clear selected node
            if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
                ImGui::IsMouseClicked(0)) {
                selected_node_name.clear();
            }
        }
        EndCanvas();
    }

    // Context menu
    context_menu_gui.draw(core, updater);
#endif

    ImGui::End();  // End window

    return issues;
}

}  // namespace fase
