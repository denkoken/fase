
#include "view.h"

#include <sstream>
#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core_util.h"

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

// Extend ImGui's operator
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

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

class CanvasController {
public:
    CanvasController(const char* label) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
        ImGui::BeginChild(label, ImVec2(0, 0), true,
                          ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoScrollWithMouse |
                                  ImGuiWindowFlags_NoMove);
        ImGui::PushItemWidth(120.0f);
    }
    ~CanvasController() {
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }
};

void DrawCanvas(const ImVec2& scroll_pos, float size) {
    const ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    const ImVec2 win_pos = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_sz = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (float x = std::fmod(scroll_pos.x, size); x < canvas_sz.x; x += size) {
        draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                           ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
    }
    for (float y = std::fmod(scroll_pos.y, size); y < canvas_sz.y; y += size) {
        draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                           ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
    }
}

}  // namespace

Content::~Content() {}

class NodeListView : public Content {
public:
    template <class... Args>
    NodeListView(Args&&... args) : Content(args...) {}
    ~NodeListView() {}

private:
    void main();
};

void NodeListView::main() {
    ImGui::Text("Nodes");
    ImGui::Separator();
    ImGui::BeginChild(label("node list"), ImVec2(), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    const std::map<std::string, Node>& nodes = core.getNodes();
    for (size_t idx = 0; idx < state.node_order.size(); idx++) {
        const std::string& node_name = state.node_order[idx];
        const Node& node = nodes.at(node_name);

        if (node.func_repr == OutputFuncStr() ||
            node.func_repr == InputFuncStr()) {
            continue;
        }

        // List component
        ImGui::PushID(label(node_name));
        std::stringstream view_ss;
        view_ss << idx << ") " << node_name;
        view_ss << " [" + node.func_repr + "]";
        if (ImGui::Selectable(label(view_ss.str()),
                              exists(state.selected_nodes, node_name))) {
            if (IsKeyPressed(ImGuiKey_Space)) {
                state.selected_nodes.push_back(node_name);
            }
            else {
                state.selected_nodes = { node_name };
            }
        }
        if (ImGui::IsItemHovered()) {
            state.hovered_node_name = node_name;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

class NodeCanvasView : public Content {
public:
    template <class... Args>
    NodeCanvasView(Args&&... args) : Content(args...) {}
    ~NodeCanvasView() {}

private:
    ImVec2 scroll_pos;
    void main();
};

void NodeCanvasView::main() {
    // TODO
    ImGui::BeginChild(label("right canvas"));
    {
        ImGui::Text("Hold middle mouse button to scroll (%f, %f)", scroll_pos.x,
                    scroll_pos.y);
        CanvasController cc(label("scrolling_region"));

        // Draw grid canvas
        DrawCanvas(scroll_pos, 64.f);
        // Draw links
        // links_gui.draw(core, updater);
        // // Draw nodes
        // node_boxes_gui.draw(core, preference);

        // Canvas scroll
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseDragging(2, 0.f)) {
            scroll_pos = scroll_pos + ImGui::GetIO().MouseDelta;
        }
        // Clear selected node
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseClicked(0)) {
            state.selected_nodes.clear();
        }
    }
}

View::View(const FaseCore& core, const TypeUtils& utils)
    : core(core), utils(utils) {
    auto add_issue_function = [this](auto&& a) { issues.emplace_back(a); };
    node_list = std::make_unique<NodeListView>(core, label, state, utils, add_issue_function);
    canvas = std::make_unique<NodeCanvasView>(core, label, state, utils, add_issue_function);
    setupMenus(add_issue_function);
}

View::~View() = default;

std::vector<Issue> View::draw(const std::string& win_title,
                              const std::string& label_suffix,
                              const std::map<std::string, Variable>& resp) {
    issues.clear();

    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(win_title.c_str(), NULL, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return {};
    }

    label.setSuffix(label_suffix);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        for (std::unique_ptr<Content>& menu : menus) {
            // draw menu.
            menu->draw(resp);
        }
        ImGui::EndMenuBar();
    }

    state.node_order.clear();

    for (const auto& pair : core.getNodes()) {
        state.node_order.push_back(std::get<0>(pair));
    }

    // Left side: Panel
    ImGui::BeginChild(label("left panel"), ImVec2(150, 0));
    {
        // Draw a list of nodes on the left side
        node_list->draw(resp);
        ImGui::EndChild();
    }
    ImGui::SameLine();

    canvas->draw(resp);

#if 0
    // TODO
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
