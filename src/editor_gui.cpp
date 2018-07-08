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

void BeginCanvas(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, IM_COL32(60, 60, 70, 200));
    ImGui::BeginChild(label, ImVec2(0, 0), true,
                      ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse |
                              ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);
}

void EndCanvas() {
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndChild();
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
        if (ImGui::MenuItem(label("Add node"))) {
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

class RunPipelineGUI {
public:
    RunPipelineGUI(LabelWrapper& label, const bool& is_pipeline_updated)
        : label(label), is_pipeline_updated(is_pipeline_updated) {}

    void draw(FaseCore* core) {
        // Run once
        if (ImGui::MenuItem(label("Run"))) {
            core->build();
            core->run();
        }

        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Run by loop
        if (!is_running) {
            if (ImGui::MenuItem(label("Run (loop)"))) {
                // Start loop
                if (core->build()) {
                    is_running = true;
                }
            }
        } else {
            if (ImGui::MenuItem(label("Stop (loop)"))) {
                // Stop
                is_running = false;
            } else {
                if (is_pipeline_updated) {
                    // Rebuild
                    core->build();
                }
                core->run();
            }
        }
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    const bool& is_pipeline_updated;

    // Private status
    bool is_running = false;
};

// Draw button and pop up window for native code
class NativeCodeGUI {
public:
    NativeCodeGUI(
            LabelWrapper& label,
            const std::map<const std::type_info*,
                           std::function<bool(const char*, const Variable&,
                                              std::string&)>>& var_generators)
        : label(label), var_generators(var_generators) {}

    void draw(FaseCore* core) {
        if (ImGui::MenuItem(label("Show code"))) {
            // Update all arguments
            updateAllArgs(core);
            // Generate native code
            native_code = core->genNativeCode();
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
    const std::map<const std::type_info*,
                   std::function<bool(const char*, const Variable&,
                                      std::string&)>>& var_generators;

    // Private status
    std::string native_code;

    void updateAllArgs(FaseCore* core) {
        const std::map<std::string, Function>& functions = core->getFunctions();
        const std::map<std::string, Node>& nodes = core->getNodes();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            const Function& function = functions.at(node.func_repr);
            const size_t n_args = node.links.size();
            for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
                const std::string& arg_name = function.arg_names[arg_idx];
                const std::type_info* arg_type = function.arg_types[arg_idx];
                if (var_generators.count(arg_type)) {
                    auto& func = var_generators.at(arg_type);
                    const Variable& var = node.arg_values[arg_idx];
                    // Get expression using GUI
                    std::string expr;
                    func(label(arg_name), var, expr);
                    // Update forcibly
                    core->setNodeArg(node_name, arg_idx, expr, var);
                } else {
                    // No GUI but no method for users to know the value changing
                }
            }
        }
    }
};

// Layout optimizer
class LayoutOptimizeGUI {
public:
    LayoutOptimizeGUI(LabelWrapper& label,
                      std::map<std::string, GuiNode>& gui_nodes)
        : label(label), gui_nodes(gui_nodes) {}

    void draw(FaseCore* core) {
        if (ImGui::MenuItem(label("Optimize layout (TODO)"))) {
            // TODO: implemented
            std::cout << "Not implemented yet" << std::endl;
        }
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    std::map<std::string, GuiNode>& gui_nodes;

    // Private status
    // [None]
};

// Node list selector
class NodeListGUI {
public:
    NodeListGUI(LabelWrapper& label, std::string& selected_node_name,
                std::string& hovered_node_name)
        : label(label),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name) {}

    void draw(FaseCore* core) {
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

private:
    // Reference to the parent's
    LabelWrapper& label;
    std::string& selected_node_name;
    std::string& hovered_node_name;

    // Private status
    // [None]
};

class NodeBoxesGUI {
public:
    NodeBoxesGUI(
            LabelWrapper& label, std::string& selected_node_name,
            std::string& hovered_node_name,
            std::map<std::string, GuiNode>& gui_nodes, const ImVec2& scroll_pos,
            const bool& is_link_creating, bool& is_any_node_moving,
            const std::map<const std::type_info*,
                           std::function<bool(const char*, const Variable&,
                                              std::string&)>>& var_generators)
        : label(label),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name),
          gui_nodes(gui_nodes),
          scroll_pos(scroll_pos),
          is_link_creating(is_link_creating),
          is_any_node_moving(is_any_node_moving),
          var_generators(var_generators) {}

    void draw(FaseCore* core) {
        // Clear cache
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
            drawNodeContent(core, node_name, node, gui_node);

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
    std::string& hovered_node_name;
    std::map<std::string, GuiNode>& gui_nodes;
    const ImVec2& scroll_pos;
    const bool& is_link_creating;
    bool& is_any_node_moving;
    const std::map<const std::type_info*,
                   std::function<bool(const char*, const Variable&,
                                      std::string&)>>& var_generators;

    // Private status
    // [None]

    void drawNodeContent(FaseCore* core, const std::string& node_name,
                         const Node& node, GuiNode& gui_node) {
        ImGui::BeginGroup();  // Lock horizontal position
        ImGui::Text("[%s] %s", node.func_repr.c_str(), node_name.c_str());

        const size_t n_args = node.links.size();
        assert(gui_node.arg_poses.size() == n_args);

        // Draw arguments
        const std::map<std::string, Function>& functions = core->getFunctions();
        const Function& function = functions.at(node.func_repr);
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            // Save argument position
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            gui_node.arg_poses[arg_idx] =
                    ImVec2(pos.x - NODE_WINDOW_PADDING.x,
                           pos.y + ImGui::GetTextLineHeight() * 0.5f);
            // Fetch from structures
            const std::string& arg_name = function.arg_names[arg_idx];
            const std::string& arg_repr = node.arg_reprs[arg_idx];
            const std::type_info* arg_type = function.arg_types[arg_idx];
            // Draw one argument
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
            ImGui::SameLine();
            if (!node.links[arg_idx].node_name.empty()) {
                // Link exists
                ImGui::Text("%s", arg_name.c_str());
            } else if (var_generators.count(arg_type)) {
                // Call registered GUI for editing
                auto& func = var_generators.at(arg_type);
                const Variable& var = node.arg_values[arg_idx];
                std::string expr;
                const bool chg = func(label(arg_name), var, expr);
                if (chg) {
                    // Update argument
                    core->setNodeArg(node_name, arg_idx, expr, var);
                }
            } else {
                // No GUI for editing
                ImGui::Text("%s [default:%s]", arg_name.c_str(),
                            arg_repr.c_str());
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
    LinksGUI(std::string& hovered_slot_name, size_t& hovered_slot_idx,
             bool& is_hovered_slot_input,
             std::map<std::string, GuiNode>& gui_nodes, bool& is_link_creating,
             const bool& is_any_node_moving)
        : hovered_slot_name(hovered_slot_name),
          hovered_slot_idx(hovered_slot_idx),
          is_hovered_slot_input(is_hovered_slot_input),
          gui_nodes(gui_nodes),
          is_link_creating(is_link_creating),
          is_any_node_moving(is_any_node_moving) {}

    void draw(FaseCore* core) {
        const std::map<std::string, Node>& nodes = core->getNodes();

        // Draw links
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            if (!gui_nodes.count(node_name)) {
                continue;  // Wait for creating GUI node
            }
            const size_t n_args = node.links.size();
            for (size_t dst_idx = 0; dst_idx < n_args; dst_idx++) {
                const std::string& src_name = node.links[dst_idx].node_name;
                const size_t& src_idx = node.links[dst_idx].arg_idx;
                if (src_name.empty() || !gui_nodes.count(src_name)) {
                    continue;  // No link or Wait for creating GUI node
                }
                // Source
                const ImVec2 s_pos = gui_nodes[src_name].getOutputSlot(src_idx);
                // Destination
                const ImVec2 d_pos = gui_nodes[node_name].getInputSlot(dst_idx);
                drawLink(s_pos, d_pos);
            }
        }

        // Search hovered slot
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const Node& node = it->second;
            if (!gui_nodes.count(node_name)) {
                continue;  // Wait for creating GUI node
            }
            GuiNode& gui_node = gui_nodes[node_name];
            const size_t n_args = node.links.size();
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
                    hovered_slot_name = node_name;
                    hovered_slot_idx = arg_idx;
                    is_hovered_slot_input = inp_hov;
                }
            }
        }

        // Link creation
        if (is_link_creating) {
            if (ImGui::IsMouseDragging(0, 0.f)) {
                // Editing
                const ImVec2 mouse_pos = ImGui::GetMousePos();
                const GuiNode& gui_node = gui_nodes.at(hovered_slot_name_prev);
                const size_t& arg_idx = hovered_slot_idx_prev;
                if (is_hovered_slot_input_prev) {
                    drawLink(mouse_pos, gui_node.getInputSlot(arg_idx));
                } else {
                    drawLink(gui_node.getOutputSlot(arg_idx), mouse_pos);
                }
            } else {
                if (hovered_slot_name.empty() ||
                    is_hovered_slot_input == is_hovered_slot_input_prev) {
                    // Canceled
                } else {
                    // Create link
                    if (is_hovered_slot_input_prev) {
                        core->addLink(hovered_slot_name, hovered_slot_idx,
                                      hovered_slot_name_prev,
                                      hovered_slot_idx_prev);
                    } else {
                        core->addLink(hovered_slot_name_prev,
                                      hovered_slot_idx_prev, hovered_slot_name,
                                      hovered_slot_idx);
                    }
                }
                is_link_creating = false;
            }
        } else {
            if (!is_any_node_moving && ImGui::IsMouseDragging(0, 0.f) &&
                !hovered_slot_name.empty()) {
                // Start creating
                is_link_creating = true;
                const Link& link =
                        nodes.at(hovered_slot_name).links[hovered_slot_idx];
                if (link.node_name.empty() || !is_hovered_slot_input) {
                    // New link
                    hovered_slot_name_prev = hovered_slot_name;
                    hovered_slot_idx_prev = hovered_slot_idx;
                    is_hovered_slot_input_prev = is_hovered_slot_input;
                } else {
                    // Edit existing link
                    hovered_slot_name_prev = link.node_name;
                    hovered_slot_idx_prev = link.arg_idx;
                    is_hovered_slot_input_prev = false;
                    core->delLink(hovered_slot_name, hovered_slot_idx);
                }
            }
        }
    }

private:
    const ImU32 LINK_COLOR = IM_COL32(200, 200, 100, 255);
    const float SLOT_HOVER_RADIUS = 8.f;

    // Reference to the parent's
    std::string& hovered_slot_name;
    size_t& hovered_slot_idx;
    bool& is_hovered_slot_input;
    std::map<std::string, GuiNode>& gui_nodes;
    bool& is_link_creating;
    const bool& is_any_node_moving;

    // Private status
    std::string hovered_slot_name_prev;
    size_t hovered_slot_idx_prev;
    bool is_hovered_slot_input_prev;

    void drawLink(const ImVec2& s_pos, const ImVec2& d_pos) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddBezierCurve(s_pos, s_pos + ImVec2(+50, 0),
                                  d_pos + ImVec2(-50, 0), d_pos, LINK_COLOR,
                                  3.0f);
    }
};

class ContextMenuGUI {
public:
    ContextMenuGUI(LabelWrapper& label, std::string& selected_node_name,
                   std::string& hovered_node_name,
                   std::string& hovered_slot_name,
                   const size_t& hovered_slot_idx,
                   const bool& is_hovered_slot_input,
                   const std::map<std::string, GuiNode>& gui_nodes)
        : label(label),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name),
          hovered_slot_name(hovered_slot_name),
          hovered_slot_idx(hovered_slot_idx),
          is_hovered_slot_input(is_hovered_slot_input),
          gui_nodes(gui_nodes) {}

    void draw(FaseCore* core) {
        // Right click
        if (ImGui::IsMouseClicked(1)) {
            if (!hovered_slot_name.empty()) {
                // Open pop up window for a slot
                selected_node_name = hovered_slot_name;
                selected_slot_name = hovered_slot_name;
                selected_slot_idx = hovered_slot_idx;
                is_selected_slot_input = is_hovered_slot_input;
                ImGui::OpenPopup(label("slot context menu"));
            } else if (!hovered_node_name.empty()) {
                // Open pop up window for a node
                selected_node_name = hovered_node_name;
                ImGui::OpenPopup(label("node context menu"));
            }
        }
        if (ImGui::BeginPopup(label("slot context menu"))) {
            // Node menu
            ImGui::Text("Link \"%s[%d][%s]\"", selected_slot_name.c_str(),
                        int(selected_slot_idx),
                        is_selected_slot_input ? "in" : "out");
            ImGui::Separator();
            if (ImGui::MenuItem(label("Clear"))) {
                if (is_selected_slot_input) {
                    core->delLink(selected_slot_name, selected_slot_idx);
                } else {
                    // TODO: implement
                    std::cout << "Not implemented yet" << std::endl;
                }
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopup(label("node context menu"))) {
            // Node menu
            ImGui::Text("Node \"%s\"", selected_node_name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem(label("Delete"))) {
                core->delNode(selected_node_name);
            }
            ImGui::EndPopup();
        }

        // Clear cache
        hovered_node_name.clear();
        hovered_slot_name.clear();
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    std::string& selected_node_name;
    std::string& hovered_node_name;
    std::string& hovered_slot_name;
    const size_t& hovered_slot_idx;
    const bool& is_hovered_slot_input;
    const std::map<std::string, GuiNode>& gui_nodes;

    // Private status
    std::string selected_slot_name;
    size_t selected_slot_idx = 0;
    bool is_selected_slot_input = false;
};

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(const std::map<const std::type_info*,
                        std::function<bool(const char*, const Variable&,
                                           std::string&)>>& var_generators)
        // Module dependencies are written here
        : node_adding_gui(label),
          run_pipeline_gui(label, is_pipeline_updated),
          native_code_gui(label, var_generators),
          layout_optimize_gui(label, gui_nodes),
          node_list_gui(label, selected_node_name, hovered_node_name),
          links_gui(hovered_slot_name, hovered_slot_idx, is_hovered_slot_input,
                    gui_nodes, is_link_creating, is_any_node_moving),
          node_boxes_gui(label, selected_node_name, hovered_node_name,
                         gui_nodes, scroll_pos, is_link_creating,
                         is_any_node_moving, var_generators),
          context_menu_gui(label, selected_node_name, hovered_node_name,
                           hovered_slot_name, hovered_slot_idx,
                           is_hovered_slot_input, gui_nodes) {}

    bool run(FaseCore* core, const std::string& win_title,
             const std::string& label_suffix);

private:
    // GUI components
    NodeAddingGUI node_adding_gui;
    RunPipelineGUI run_pipeline_gui;
    NativeCodeGUI native_code_gui;
    LayoutOptimizeGUI layout_optimize_gui;
    NodeListGUI node_list_gui;
    LinksGUI links_gui;
    NodeBoxesGUI node_boxes_gui;
    ContextMenuGUI context_menu_gui;

    // Common status
    LabelWrapper label;  // Label wrapper for suffix to generate unique label
    std::string selected_node_name;
    std::string hovered_node_name;
    std::string hovered_slot_name;
    size_t hovered_slot_idx;
    bool is_hovered_slot_input;
    std::map<std::string, GuiNode> gui_nodes;
    ImVec2 scroll_pos = ImVec2(0.0f, 0.0f);
    bool is_link_creating = false;
    bool is_any_node_moving = false;
    bool is_pipeline_updated = false;
    size_t prev_n_nodes = 0;
    size_t prev_n_links = 0;

    void updateGuiNodes(FaseCore* core) {
        // Clear cache
        is_pipeline_updated = false;

        const std::map<std::string, Node>& nodes = core->getNodes();
        size_t n_nodes = nodes.size();
        size_t n_links = 0;
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const size_t n_args = it->second.links.size();
            // Check GUI node existence
            if (!gui_nodes.count(node_name)) {
                // Create new node and allocate for link slots
                gui_nodes[node_name].alloc(n_args);
                is_pipeline_updated = true;
            }
            // Count up links
            const auto& links = it->second.links;
            for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
                if (!links[arg_idx].node_name.empty()) {
                    n_links++;
                }
            }
        }
        // Detect changes by the numbers of nodes and links
        if (n_nodes != prev_n_nodes || n_links != prev_n_links) {
            is_pipeline_updated = true;
            prev_n_nodes = n_nodes;
            prev_n_links = n_links;
        }
    }
};

bool GUIEditor::Impl::run(FaseCore* core, const std::string& win_title,
                          const std::string& label_suffix) {
    // Create ImGui window
    ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(win_title.c_str(), NULL, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return true;
    }

    // Update GUI nodes
    updateGuiNodes(core);

    // Update label suffix
    label.setSuffix(label_suffix);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        // Menu to add new node
        node_adding_gui.draw(core);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to run
        run_pipeline_gui.draw(core);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to show native code
        native_code_gui.draw(core);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to optimize the node layout
        layout_optimize_gui.draw(core);
        ImGui::EndMenuBar();
    }

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
        ImGui::Text("Hold right mouse button to scroll (%f, %f)", scroll_pos.x,
                    scroll_pos.y);
        BeginCanvas(label("scrolling_region"));
        {
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
        }
        EndCanvas();
    }

    // Context menu
    context_menu_gui.draw(core);

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
