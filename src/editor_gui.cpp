#include "editor.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <sstream>

namespace fase {

namespace {

// Visible GUI node
struct GuiNode {
    ImVec2 pos;
    ImVec2 size;
    std::vector<ImVec2> arg_poses;
    std::vector<char> arg_inp_hovered;
    std::vector<char> arg_out_hovered;

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

// Label wrapper for suffix
class LabelWrapper {
public:
    void setSuffix(const std::string& s) {
        suffix = s;
    }
    const char* operator()(const std::string& label) {
        last_label = label + suffix;
        return last_label.c_str();
    }

private:
    std::string suffix;
    std::string last_label;  // temporary storage to return char*
};

// Extend ImGui's operator
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline ImVec2 operator*(const ImVec2& lhs, const float& rhs) {
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}
inline float Length(const ImVec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
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

// Draw canvas with grids
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

// Draw button and pop up window for node adding
class NodeAddingGUI {
public:
    NodeAddingGUI(LabelWrapper& label) : label(label) {}

    void draw(FaseCore* core) {
        if (ImGui::Button(label("Add node"))) {
            ImGui::OpenPopup(label("Popup: Add node"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: Add node"), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened) {
                closePopup();  // Behavior of close button
            }

            // Extract function representations
            updateFuncReprs(core->getFunctions());

            // Input elements
            ImGui::InputText(label("Node name (ID)"), name_buf,
                             sizeof(name_buf));
            const bool enter_pushed = IsKeyPressedOnItem(ImGuiKey_Enter);
            Combo(label("Function"), &curr_idx, func_reprs);
            if (!error_msg.empty()) {
                ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
            }
            if (ImGui::Button(label("OK")) || enter_pushed) {
                // Create new node
                if (core->addNode(name_buf, func_reprs[size_t(curr_idx)])) {
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

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Reference to the parent's
    LabelWrapper& label;

    // Private status
    std::vector<std::string> func_reprs;
    char name_buf[1024] = "";
    int curr_idx = 0;
    std::string error_msg;

    void updateFuncReprs(const std::map<std::string, Function>& functions) {
        if (functions.size() != func_reprs.size()) {
            // Create all again
            func_reprs.clear();
            for (auto it = functions.begin(); it != functions.end(); it++) {
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

// Node list selector
class NodeListGUI {
public:
    NodeListGUI(LabelWrapper& label, std::string& selected_node_name)
        : label(label), selected_node_name(selected_node_name) {}

    void draw(FaseCore* core) {
        // Clear cache
        hovered_node_name = false;

        ImGui::Text("Nodes");
        ImGui::Separator();
        ImGui::BeginChild(label("node list"), ImVec2(), false,
                          ImGuiWindowFlags_HorizontalScrollbar);
        const std::map<std::string, Node>& nodes = core->getNodes();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;

            // List component
            ImGui::PushID(label(node_name));
            std::string view_lavel = node_name + " [" + node.func_repr + "]";
            if (ImGui::Selectable(label(view_lavel),
                                  node_name == selected_node_name)) {
                selected_node_name = node_name;
            }
            if (ImGui::IsItemHovered()) {
                hovered_node_name = node_name;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    const std::string& getHoveredNode() const {
        return hovered_node_name;
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    std::string& selected_node_name;

    // Private status
    std::string hovered_node_name;
};

class NodeBoxesGUI {
public:
    NodeBoxesGUI(
            LabelWrapper& label, std::string& selected_node_name,
            std::map<std::string, GuiNode>& gui_nodes, const ImVec2& scroll_pos,
            const bool& is_link_creating, bool& is_any_node_moving,
            const std::map<const std::type_info*,
                           std::function<void(const char*, const Variable&)>>&
                    var_generators)
        : label(label),
          selected_node_name(selected_node_name),
          gui_nodes(gui_nodes),
          scroll_pos(scroll_pos),
          is_link_creating(is_link_creating),
          is_any_node_moving(is_any_node_moving),
          var_generators(var_generators) {}

    void draw(FaseCore* core) {
        // Clear cache
        hovered_node_name = false;
        is_any_node_moving = false;

        const ImVec2 canvas_offset = ImGui::GetCursorScreenPos() + scroll_pos;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->ChannelsSplit(2);
        const std::map<std::string, Node>& nodes = core->getNodes();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            if (!gui_nodes.count(node_name)) {
                continue;  // Wait for creating GUI node
            }
            GuiNode& gui_node = gui_nodes.at(node_name);

            ImGui::PushID(label(node_name));
            const ImVec2 node_rect_min = canvas_offset + gui_node.pos;
            const bool any_active_old = ImGui::IsAnyItemActive();

            // Draw node contents first
            draw_list->ChannelsSetCurrent(1);  // Foreground
            ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
            drawNodeContent(node_name, node, gui_node, core->getFunctions());

            // Fit to content size
            gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

            // Draw node box
            draw_list->ChannelsSetCurrent(0);  // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            drawNodeBox(node_rect_min, gui_node.size,
                        (selected_node_name == node_name));

            // Draw link slot
            drawLinkSlots(gui_node);

            // Selection
            if (!any_active_old && ImGui::IsAnyItemActive()) {
                selected_node_name = node_name;
            }
            if (ImGui::IsItemHovered()) {
                hovered_node_name = node_name;
            }
            // Scroll
            if (!is_link_creating && ImGui::IsItemActive() &&
                ImGui::IsMouseDragging(0, 0.f)) {
                gui_node.pos = gui_node.pos + ImGui::GetIO().MouseDelta;
                is_any_node_moving = true;
            }

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();
    }

private:
    const float SLOT_RADIUS = 4.f;
    const float SLOT_SPACING = 3.f;
    const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 8.f);
    const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
    const ImU32 BG_NML_COLOR = IM_COL32(60, 60, 60, 255);
    const ImU32 BG_ACT_COLOR = IM_COL32(75, 75, 75, 255);
    const ImU32 SLOT_NML_COLOR = IM_COL32(240, 240, 150, 150);
    const ImU32 SLOT_ACT_COLOR = IM_COL32(255, 255, 100, 255);

    // Reference to the parent's
    LabelWrapper& label;
    std::string& selected_node_name;
    std::map<std::string, GuiNode>& gui_nodes;
    const ImVec2& scroll_pos;
    const bool& is_link_creating;
    bool& is_any_node_moving;
    const std::map<const std::type_info*,
                   std::function<void(const char*, const fase::Variable&)>>&
            var_generators;

    // Private status
    std::string hovered_node_name;

    void drawNodeContent(const std::string& node_name, const Node& node,
                         GuiNode& gui_node,
                         const std::map<std::string, Function>& functions) {
        ImGui::BeginGroup();  // Lock horizontal position
        ImGui::Text("[%s] %s", node.func_repr.c_str(), node_name.c_str());

        const size_t n_args = node.links.size();
        assert(gui_node.arg_poses.size() == n_args);

        // Draw arguments
        const Function& function = functions.at(node.func_repr);
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            // Save argument position
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            gui_node.arg_poses[arg_idx] =
                    ImVec2(pos.x - NODE_WINDOW_PADDING.x,
                           pos.y + ImGui::GetTextLineHeight() * 0.5f);
            // Fetch from structures
            const std::string& arg_name = function.arg_names[arg_idx];
            const std::type_info* arg_type = function.arg_types[arg_idx];
            const std::string& arg_type_repr = function.arg_type_reprs[arg_idx];
            const std::string& arg_repr = node.arg_reprs[arg_idx];
            // Draw one argument
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
            ImGui::SameLine();
            if (node.links[arg_idx].node_name.empty() &&
                var_generators.count(arg_type)) {
                // No link exists
                auto func = var_generators.at(arg_type);
                // Call registered GUI for editing
                func(label(arg_name), node.arg_values[arg_idx]);
            } else {
                // Link exists or No GUI for editing
                ImGui::Text("%s [%s]", arg_repr.c_str(), arg_type_repr.c_str());
            }
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
        }
        ImGui::EndGroup();
    }

    void drawNodeBox(const ImVec2& node_rect_min, const ImVec2& node_size,
                     bool is_active) {
        const ImVec2 node_rect_max = node_rect_min + node_size;
        ImGui::InvisibleButton(label("node"), node_size);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImU32 bg_col = is_active ? BG_ACT_COLOR : BG_NML_COLOR;
        draw_list->AddRectFilled(node_rect_min, node_rect_max, bg_col, 4.f);
        draw_list->AddRect(node_rect_min, node_rect_max, BORDER_COLOR, 4.f);
    }

    void drawLinkSlots(const GuiNode& gui_node) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const size_t n_args = gui_node.arg_poses.size();
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            // Get slot position
            const ImVec2 inp_slot = gui_node.getInputSlot(arg_idx);
            const ImVec2 out_slot = gui_node.getOutputSlot(arg_idx);
            // Get hovered conditions
            const char& inp_hov = gui_node.arg_inp_hovered[arg_idx];
            const char& out_hov = gui_node.arg_out_hovered[arg_idx];
            // Draw
            const ImU32 inp_col = inp_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
            const ImU32 out_col = out_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
            draw_list->AddCircleFilled(inp_slot, SLOT_RADIUS, inp_col);
            draw_list->AddCircleFilled(out_slot, SLOT_RADIUS, out_col);
        }
    }
};

class LinksGUI {
public:
    LinksGUI(LabelWrapper& label, std::map<std::string, GuiNode>& gui_nodes,
             bool& is_link_creating, const bool& is_any_node_moving)
        : label(label),
          gui_nodes(gui_nodes),
          is_link_creating(is_link_creating),
          is_any_node_moving(is_any_node_moving) {}

    void draw(FaseCore* core) {
        const std::map<std::string, Node>& nodes = core->getNodes();

        // Draw links
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            const size_t n_args = node.links.size();
            for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
                const std::string& src_name = node.links[arg_idx].node_name;
                if (src_name.empty()) {
                    continue;
                }
                const ImVec2 s_pos = gui_nodes[src_name].getOutputSlot(arg_idx);
                const ImVec2 d_pos = gui_nodes[node_name].getInputSlot(arg_idx);
                drawLink(s_pos, d_pos);
            }
        }

        // Search hovered slot
        std::string hovered_node_name;
        size_t hovered_arg_idx;
        bool is_hovered_input;
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            const size_t n_args = node.links.size();
            GuiNode& gui_node = gui_nodes[node_name];
            for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
                // Get slot position
                const ImVec2 inp_slot = gui_node.getInputSlot(arg_idx);
                const ImVec2 out_slot = gui_node.getOutputSlot(arg_idx);
                // Get hovered conditions
                char& inp_hov = gui_node.arg_inp_hovered[arg_idx];
                char& out_hov = gui_node.arg_out_hovered[arg_idx];
                // Update hovered condition
                inp_hov = (Length(mouse_pos - inp_slot) <= SLOT_HOVER_RADIUS);
                out_hov = (Length(mouse_pos - out_slot) <= SLOT_HOVER_RADIUS);
                if (inp_hov || out_hov) {
                    // Save the last one
                    hovered_node_name = node_name;
                    hovered_arg_idx = arg_idx;
                    is_hovered_input = inp_hov;
                }
            }
        }

        // Link creation
        if (is_link_creating) {
            if (ImGui::IsMouseDragging(0, 0.f)) {
                // Editing
                const ImVec2 mouse_pos = ImGui::GetMousePos();
                const GuiNode& gui_node = gui_nodes.at(hovered_node_name_prev);
                const size_t& arg_idx = hovered_arg_idx_prev;
                if (is_hovered_input_prev) {
                    drawLink(mouse_pos, gui_node.getInputSlot(arg_idx));
                } else {
                    drawLink(gui_node.getOutputSlot(arg_idx), mouse_pos);
                }
            } else {
                if (hovered_node_name.empty() ||
                    is_hovered_input == is_hovered_input_prev) {
                    // Canceled
                } else {
                    // Create link
                    if (is_hovered_input_prev) {
                        core->addLink(hovered_node_name, hovered_arg_idx,
                                      hovered_node_name_prev,
                                      hovered_arg_idx_prev);
                    } else {
                        core->addLink(hovered_node_name_prev,
                                      hovered_arg_idx_prev, hovered_node_name,
                                      hovered_arg_idx);
                    }
                }
                is_link_creating = false;
            }
        } else {
            if (!is_any_node_moving && ImGui::IsMouseDragging(0, 0.f) &&
                !hovered_node_name.empty()) {
                // Start creating
                is_link_creating = true;
                hovered_node_name_prev = hovered_node_name;
                hovered_arg_idx_prev = hovered_arg_idx;
                is_hovered_input_prev = is_hovered_input;
            }
        }
    }

private:
    const ImU32 LINK_COLOR = IM_COL32(200, 200, 100, 255);
    const float SLOT_HOVER_RADIUS = 8.f;

    // Reference to the parent's
    LabelWrapper& label;
    std::map<std::string, GuiNode>& gui_nodes;
    bool& is_link_creating;
    const bool& is_any_node_moving;

    // Private status
    std::string hovered_node_name_prev;
    size_t hovered_arg_idx_prev;
    bool is_hovered_input_prev;

    void drawLink(const ImVec2& s_pos, const ImVec2& d_pos) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddBezierCurve(s_pos, s_pos + ImVec2(+50, 0),
                                  d_pos + ImVec2(-50, 0), d_pos, LINK_COLOR,
                                  3.0f);
    }
};

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(const std::map<const std::type_info*,
                        std::function<void(const char*, const Variable&)>>&
                 var_generators)
        // Module dependencies are written here
        : node_adding_gui(label),
          node_list_gui(label, selected_node_name),
          links_gui(label, gui_nodes, is_link_creating, is_any_node_moving),
          node_boxes_gui(label, selected_node_name, gui_nodes, scroll_pos,
                         is_link_creating, is_any_node_moving, var_generators) {
    }

    bool run(FaseCore* core, const std::string& win_title,
             const std::string& label_suffix);

private:
    // GUI components
    NodeAddingGUI node_adding_gui;
    NodeListGUI node_list_gui;
    LinksGUI links_gui;
    NodeBoxesGUI node_boxes_gui;

    // Label wrapper for suffix to generate unique label
    LabelWrapper label;

    // Common status
    std::string selected_node_name;
    std::map<std::string, GuiNode> gui_nodes;
    ImVec2 scroll_pos = ImVec2(0.0f, 0.0f);
    bool is_link_creating = false;
    bool is_any_node_moving = false;
};

bool GUIEditor::Impl::run(FaseCore* core, const std::string& win_title,
                          const std::string& label_suffix) {
    // Create ImGui window
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(win_title.c_str())) {
        ImGui::End();
        return true;
    }

    // Update GUI nodes
    const std::map<std::string, Node>& nodes = core->getNodes();
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        const std::string& node_name = it->first;
        if (!gui_nodes.count(node_name)) {
            // Create new node and allocate for link slots
            const size_t n_args = it->second.links.size();
            gui_nodes[node_name].alloc(n_args);
        }
    }

    // Update label suffix
    label.setSuffix(label_suffix);

    // Left side: Panel
    ImGui::BeginChild(label("left panel"), ImVec2(150, 0));
    {
        // Button to add new node
        node_adding_gui.draw(core);
        // Spacing
        ImGui::Dummy(ImVec2(0, 5));
        // Draw a list of nodes on the left side
        node_list_gui.draw(core);
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Right side: Canvas
    ImGui::BeginChild(label("right canvas"));
    {
        ImGui::Text("Hold right mouse button to scroll (%f, %f)", scroll_pos.x,
                    scroll_pos.y);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg,
                              IM_COL32(60, 60, 70, 200));
        ImGui::BeginChild(
                label("scrolling_region"), ImVec2(0, 0), true,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImGui::PushItemWidth(120.0f);

        // Draw grid canvas
        DrawCanvas(scroll_pos, 64.f);
        // Draw links
        links_gui.draw(core);
        // Draw nodes
        node_boxes_gui.draw(core);

        // Canvas scroll
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseDragging(1, 0.f)) {
            scroll_pos = scroll_pos + ImGui::GetIO().MouseDelta;
        }
        // Clear selected node
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseClicked(0)) {
            selected_node_name.clear();
        }

        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }

    ImGui::End();  // End window
    return true;
}

// ------------------------------- pImpl pattern -------------------------------
GUIEditor::GUIEditor() : impl(new GUIEditor::Impl(var_generators)) {}
GUIEditor::~GUIEditor() {}
bool GUIEditor::run(FaseCore* core, const std::string& win_title,
                    const std::string& label_suffix) {
    return impl->run(core, win_title, label_suffix);
}

}  // namespace fase
