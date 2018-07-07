#include "editor.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <sstream>

namespace fase {

namespace {

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
inline ImVec2 operator*(const ImVec2& lhs, const float& rhs) {
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
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

// Visible GUI node
struct GuiNode {
    ImVec2 pos;
    ImVec2 size;
    std::vector<ImVec2> arg_poses;

    ImVec2 getInputSlot(const size_t idx) const {
        return arg_poses[idx];
    }
    ImVec2 getOutputSlot(const size_t idx) const {
        return ImVec2(arg_poses[idx].x + size.x, arg_poses[idx].y);
    }
};

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

class NodeBoxGUI {
public:
    NodeBoxGUI(
            LabelWrapper& label, std::string& selected_node_name,
            std::map<std::string, GuiNode>& gui_nodes, const ImVec2& scroll_pos,
            const std::map<const std::type_info*,
                           std::function<void(const char*, const Variable&)>>&
                    var_generators)
        : label(label),
          selected_node_name(selected_node_name),
          gui_nodes(gui_nodes),
          scroll_pos(scroll_pos),
          var_generators(var_generators) {}

    void draw(FaseCore* core) {
        // Clear cache
        hovered_node_name = false;

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
            drawNodeContents(node_name, node, gui_node, core->getFunctions(),
                             label);

            // Fit to content size
            gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

            // Draw node box
            draw_list->ChannelsSetCurrent(0);  // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            drawNodeBox(node_rect_min, gui_node.size, label,
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
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                gui_node.pos = gui_node.pos + ImGui::GetIO().MouseDelta;
            }
            ImGui::PopID();
        }
        draw_list->ChannelsMerge();
    }

private:
    const float NODE_SLOT_RADIUS = 4.f;
    const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 8.f);
    const ImU32 BG_COLOR = IM_COL32(60, 60, 60, 255);
    const ImU32 BG_ACTICE_COLOR = IM_COL32(75, 75, 75, 255);
    const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
    const float SLOT_SPACING = 3.f;
    const ImU32 SLOT_COLOR = IM_COL32(240, 240, 150, 150);

    // Reference to the parent's
    LabelWrapper& label;
    std::string& selected_node_name;
    std::map<std::string, GuiNode>& gui_nodes;
    const ImVec2& scroll_pos;
    const std::map<const std::type_info*,
                   std::function<void(const char*, const fase::Variable&)>>&
            var_generators;

    // Private status
    std::string hovered_node_name;

    void drawNodeContents(const std::string& node_name, const Node& node,
                          GuiNode& gui_node,
                          const std::map<std::string, Function>& functions,
                          LabelWrapper& label) {
        ImGui::BeginGroup();  // Lock horizontal position
        ImGui::Text("[%s] %s", node.func_repr.c_str(), node_name.c_str());

        const size_t n_args = node.links.size();
        gui_node.arg_poses.resize(n_args);

        // Draw arguments
        const Function& function = functions.at(node.func_repr);
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            // Save argument position
            gui_node.arg_poses[arg_idx] = ImGui::GetCursorScreenPos();
            gui_node.arg_poses[arg_idx].x -= NODE_WINDOW_PADDING.x;
            gui_node.arg_poses[arg_idx].y += ImGui::GetTextLineHeight() * 0.5f;

            const std::string& arg_name = function.arg_names[arg_idx];
            const std::type_info* arg_type = function.arg_types[arg_idx];
            const std::string& arg_type_repr = function.arg_type_reprs[arg_idx];
            const std::string& arg_repr = node.arg_reprs[arg_idx];

            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
            ImGui::SameLine();
            if (node.links[arg_idx].node_name.empty()) {
                // No link exists
                if (var_generators.count(arg_type)) {
                    // Call registered GUI for editing
                    auto func = var_generators.at(arg_type);
                    func(label(arg_name), node.arg_values[arg_idx]);
                } else {
                    // No GUI for editing
                    ImGui::Text("[%s] %s", arg_type_repr.c_str(),
                                arg_repr.c_str());
                }
                ImGui::SameLine();
            }
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
        }
        ImGui::EndGroup();
    }

    void drawNodeBox(const ImVec2& node_rect_min, const ImVec2& node_size,
                     LabelWrapper& label, bool is_active) {
        const ImVec2 node_rect_max = node_rect_min + node_size;
        ImGui::InvisibleButton(label("node"), node_size);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        if (is_active) {
            draw_list->AddRectFilled(node_rect_min, node_rect_max,
                                     BG_ACTICE_COLOR, 4.f);
        } else {
            draw_list->AddRectFilled(node_rect_min, node_rect_max, BG_COLOR,
                                     4.f);
        }
        draw_list->AddRect(node_rect_min, node_rect_max, BORDER_COLOR, 4.f);
    }

    void drawLinkSlots(const GuiNode& gui_node) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (size_t s_idx = 0; s_idx < gui_node.arg_poses.size(); s_idx++) {
            draw_list->AddCircleFilled(gui_node.getInputSlot(s_idx),
                                       NODE_SLOT_RADIUS, SLOT_COLOR);
            draw_list->AddCircleFilled(gui_node.getOutputSlot(s_idx),
                                       NODE_SLOT_RADIUS, SLOT_COLOR);
        }
    }
};

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(const std::map<const std::type_info*,
                        std::function<void(const char*, const Variable&)>>&
                 var_generators)
        : node_adding_gui(label),
          node_list_gui(label, selected_node_name),
          node_box_gui(label, selected_node_name, gui_nodes, scroll_pos,
                       var_generators) {}

    bool run(FaseCore* core, const std::string& win_title,
             const std::string& label_suffix);

private:
    // GUI components
    NodeAddingGUI node_adding_gui;
    NodeListGUI node_list_gui;
    NodeBoxGUI node_box_gui;

    // Label wrapper for suffix to generate unique label
    LabelWrapper label;

    // Common status
    std::string selected_node_name;
    std::map<std::string, GuiNode> gui_nodes;
    ImVec2 scroll_pos = ImVec2(0.0f, 0.0f);
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
            gui_nodes[node_name] = {};  // empty
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
        // Draw nodes
        node_box_gui.draw(core);

        // Canvas scroll
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseDragging(1, 0.0f)) {
            scroll_pos = scroll_pos + ImGui::GetIO().MouseDelta;
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
