#include "editor.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <sstream>

#include "core_util.h"
#include "editor_gui_view.h"

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

struct GUIPreference {
    bool auto_layout = false;
    int priority_min = -10;
    int priority_max = 10;
};

template <class T, class Filter>
T SubGroup(const T& group, const Filter& f) {
    T dst;
    auto i = std::begin(group), end = std::end(group);
    while (i != end) {
        i = std::find_if(i, end, f);

        if (i != end) {
            dst.insert(*i);
            i++;
        }
    }
    return dst;
}

template <typename Key, typename T>
std::vector<Key> GetKeys(const std::map<Key, T>& map) {
    std::vector<Key> keys;
    for (const auto& pair : map) {
        keys.push_back(std::get<0>(pair));
    }
    return keys;
}

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

// Search connected output slots of Fase's node
void FindConnectedOutSlots(const std::map<std::string, Node>& nodes,
                           const std::string& node_name, const size_t& arg_idx,
                           std::vector<std::pair<std::string, size_t>>& slots) {
    slots.clear();
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        const std::string& src_node_name = it->first;
        const Node& src_node = it->second;
        const size_t n_args = src_node.links.size();
        for (size_t src_arg_idx = 0; src_arg_idx < n_args; src_arg_idx++) {
            const Link& link = src_node.links[src_arg_idx];
            if (link.node_name == node_name && link.arg_idx == arg_idx) {
                slots.emplace_back(src_node_name, src_arg_idx);
            }
        }
    }
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
    NodeAddingGUI(LabelWrapper& label, bool& request_add_node)
        : label(label), request_add_node(request_add_node) {}

    void draw(FaseCore* core, const std::function<void()>& updater) {
        if (ImGui::MenuItem(label("Add node")) || request_add_node) {
            request_add_node = false;  // Clear request
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
                    updater();
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
    bool& request_add_node;

    // Private status
    std::vector<std::string> func_reprs;
    char name_buf[1024] = "";
    int curr_idx = 0;
    std::string error_msg;

    // Private methods
    void setDefaultNodeName(FaseCore* core) {
        std::stringstream ss;
        ss << "node_" << core->getNodes().size();
        strncpy(name_buf, ss.str().c_str(), sizeof(name_buf));
    }

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
    RunPipelineGUI(LabelWrapper& label, const bool& is_pipeline_updated,
                   std::map<std::string, ResultReport>* p_reports)
        : label(label),
          is_pipeline_updated(is_pipeline_updated),
          p_reports(p_reports) {}

    void draw(FaseCore* core) {
        if (!is_running && ImGui::BeginMenu("Run")) {
            // Run once
            if (ImGui::MenuItem(label("Run")) && !is_running) {
                core->build(multi, report);
                *p_reports = core->run();
            }
            ImGui::Dummy(ImVec2(5, 0));  // Spacing
            // Run by loop
            if (!is_running) {
                if (ImGui::MenuItem(label("Run (loop)"))) {
                    // Start loop
                    if (core->build(multi, report)) {
                        is_running = true;
                    }
                }
            }
            if (ImGui::MenuItem(label("Multi Build"), NULL, multi)) {
                multi = !multi;
            }
            if (ImGui::MenuItem(label("Reporting"), NULL, report)) {
                report = !report;
            }
            ImGui::EndMenu();
        } else if (is_running) {
            if (ImGui::MenuItem(label("Stop!"))) {
                // Stop
                is_running = false;
            } else {
                if (is_pipeline_updated) {
                    // Rebuild
                    core->build(multi, report);
                }
                *p_reports = core->run();
            }
        }
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    const bool& is_pipeline_updated;
    std::map<std::string, ResultReport>* p_reports;

    // Private status
    bool is_running = false;
    bool multi = false;
    bool report = true;
};

// Draw button and pop up window for native code
class NativeCodeGUI {
public:
    NativeCodeGUI(LabelWrapper& label) : label(label) {}

    void draw(FaseCore* core, const TypeUtils& utils) {
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

// Draw button and pop up window for native code
class AddInOutputGUI {
public:
    AddInOutputGUI(LabelWrapper& label) : label(label) {}

    void draw(FaseCore* core, std::function<void()>& updater) {
        if (ImGui::MenuItem(label("Add input/output"))) {
            // Open pop up window
            ImGui::OpenPopup(label("Popup: AddInOutputGUI"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: AddInOutputGUI"), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            ImGui::InputText(label("Name"), buf, sizeof(buf));

            if (!error_msg.empty()) {
                ImGui::TextColored(ImVec4(255, 0, 0, 255), "%s",
                                   error_msg.c_str());
            }

            if (ImGui::Button(label("Make input"))) {
                if (core->addInput(buf)) {
                    ImGui::CloseCurrentPopup();
                    updater();
                    error_msg = "";
                } else {
                    error_msg = "Invalid Name";  // Failed
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(label("Make output"))) {
                if (core->addOutput(buf)) {
                    ImGui::CloseCurrentPopup();
                    updater();
                    error_msg = "";
                } else {
                    error_msg = "Invalid Name";  // Failed
                }
            }

            if (core->getNodes().at(InputNodeStr()).links.size() > 0) {
                if (ImGui::Button(label("del Input"))) {
                    size_t idx =
                            core->getNodes().at(InputNodeStr()).links.size() -
                            1;
                    core->delInput(idx);
                    updater();
                }
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
            }
            if (core->getNodes().at(OutputNodeStr()).links.size() > 0) {
                if (ImGui::Button(label("del Output"))) {
                    size_t idx =
                            core->getNodes().at(OutputNodeStr()).links.size() -
                            1;
                    core->delOutput(idx);
                    updater();
                }
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
            }

            ImGui::EndPopup();
        }
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Reference to the parent's
    LabelWrapper& label;

    char buf[64];
    std::string error_msg;
};

// Layout optimizer
class LayoutOptimizeGUI {
public:
    LayoutOptimizeGUI(LabelWrapper& label,
                      std::map<std::string, GuiNode>& gui_nodes)
        : label(label), gui_nodes(gui_nodes), destinations() {}

    void draw(FaseCore* core, bool auto_layout) {
        if (auto_layout) {
            SetDestinations(core);
        } else if (ImGui::MenuItem(label("Optimize layout"))) {
            SetDestinations(core);
        }

        for (auto& pair : gui_nodes) {
            if (!fase::exists(destinations, std::get<0>(pair))) {
                continue;
            }
            std::get<1>(pair).pos = destinations[std::get<0>(pair)] * .3f +
                                    std::get<1>(pair).pos * .7f;

            if (std::abs(destinations[std::get<0>(pair)].x -
                         std::get<1>(pair).pos.x) < 1.f &&
                std::abs(destinations[std::get<0>(pair)].y -
                         std::get<1>(pair).pos.y) < 1.f) {
                destinations.erase(std::get<0>(pair));
            }
        }
    }

private:
    // Reference to the parent's
    LabelWrapper& label;
    std::map<std::string, GuiNode>& gui_nodes;

    std::map<std::string, ImVec2> destinations;

    static constexpr float START_X = 30;
    static constexpr float START_Y = 10;
    static constexpr float INTERVAL_X = 50;
    static constexpr float INTERVAL_Y = 10;

    void SetDestinations(FaseCore* core) {
        auto names = GetCallOrder(core->getNodes());

        float baseline_y = 0;
        float x = START_X, y = START_Y, maxy = 0;
        ImVec2 component_size =
                ImGui::GetWindowSize() - ImGui::GetItemRectSize();
        for (auto& name_set : names) {
            float maxx = 0;
            for (auto& name : name_set) {
                maxx = std::max(maxx, gui_nodes[name].size.x);
            }
            if (x + maxx > component_size.x) {
                x = START_X;
                baseline_y = maxy + INTERVAL_Y * 0.7f + baseline_y;
            }
            for (auto& name : name_set) {
                destinations[name].y = y + baseline_y;
                destinations[name].x = x;
                y += gui_nodes[name].size.y + INTERVAL_Y;
            }
            maxy = std::max(maxy, y);
            y = START_Y;
            x += maxx + INTERVAL_X;
        }
    }
};

// FaseCore saver
class SaveGUI {
public:
    SaveGUI(LabelWrapper& label) : label(label) {}

    void draw(FaseCore* core, const TypeUtils& utils) {
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
                if (SaveFaseCore(filename_buf, *core, utils)) {
                    ImGui::CloseCurrentPopup();
                    error_msg = "";
                } else {
                    error_msg = "Failed to save pipeline";  // Failed
                }
            }

            ImGui::EndPopup();
        }
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Reference to the parent's
    LabelWrapper& label;

    std::string error_msg;
    char filename_buf[1024] = "fase_save.txt";
};

// FaseCore saver
class LoadGUI {
public:
    LoadGUI(LabelWrapper& label) : label(label) {}

    void draw(FaseCore* core, const TypeUtils& utils,
              std::function<void()>& updater) {
        if (ImGui::MenuItem(label("Load"))) {
            ImGui::OpenPopup(label("Popup: Load pipeline"));
        }
        bool opened = true;
        if (ImGui::BeginPopupModal(label("Popup: Load pipeline"), &opened,
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
                if (LoadFaseCore(filename_buf, core, utils)) {
                    ImGui::CloseCurrentPopup();
                    error_msg = "";
                    updater();
                } else {
                    error_msg = "Failed to load pipeline";  // Failed
                }
            }

            ImGui::EndPopup();
        }
    }

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    // Reference to the parent's
    LabelWrapper& label;

    std::string error_msg;
    char filename_buf[1024] = "fase_save.txt";
};

// Node list selector
class NodeListGUI {
public:
    NodeListGUI(LabelWrapper& label, const std::vector<std::string>& node_order,
                std::string& selected_node_name, std::string& hovered_node_name)
        : label(label),
          node_order(node_order),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name) {}

    void draw(FaseCore* core) {
        ImGui::Text("Nodes");
        ImGui::Separator();
        ImGui::BeginChild(label("node list"), ImVec2(), false,
                          ImGuiWindowFlags_HorizontalScrollbar);
        const std::map<std::string, Node>& nodes = core->getNodes();
        for (size_t order_idx = 0; order_idx < node_order.size(); order_idx++) {
            const std::string& node_name = node_order[order_idx];
            const Node& node = nodes.at(node_name);

            if (node.func_repr == OutputFuncStr() ||
                node.func_repr == InputFuncStr()) {
                continue;
            }

            // List component
            ImGui::PushID(label(node_name));
            std::stringstream view_ss;
            view_ss << order_idx << ") " << node_name;
            view_ss << " [" + node.func_repr + "]";
            if (ImGui::Selectable(label(view_ss.str()),
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
    const std::vector<std::string>& node_order;
    std::string& selected_node_name;
    std::string& hovered_node_name;

    // Private status
    // [None]
};

class NodeBoxesGUI {
public:
    NodeBoxesGUI(LabelWrapper& label,
                 const std::vector<std::string>& node_order,
                 std::string& selected_node_name,
                 std::string& hovered_node_name,
                 std::map<std::string, GuiNode>& gui_nodes,
                 const ImVec2& scroll_pos, const bool& is_link_creating,
                 const std::map<const std::type_info*, VarEditor>& var_editors)
        : label(label),
          node_order(node_order),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name),
          gui_nodes(gui_nodes),
          scroll_pos(scroll_pos),
          is_link_creating(is_link_creating),
          var_editors(var_editors) {}

    void draw(FaseCore* core, const GUIPreference& preference) {
        const ImVec2 canvas_offset = ImGui::GetCursorScreenPos() + scroll_pos;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->ChannelsSplit(2);
        const std::map<std::string, Node>& nodes = core->getNodes();
        for (size_t order_idx = 0; order_idx < node_order.size(); order_idx++) {
            const std::string& node_name = node_order[order_idx];
            const Node& node = nodes.at(node_name);
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
            drawNodeContent(core, node_name, node, gui_node, order_idx,
                            preference);

            // Fit to content size
            gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

            // Draw node box
            draw_list->ChannelsSetCurrent(0);  // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            drawNodeBox(node_rect_min, gui_node.size,
                        (selected_node_name == node_name), order_idx);

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
            }

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();
    }

private:
    const float SLOT_RADIUS = 4.f;
    const float SLOT_SPACING = 3.f;
    const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 6.f);
    const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
    const ImU32 BG_NML_COLOR = IM_COL32(60, 60, 60, 255);
    const ImU32 BG_ACT_COLOR = IM_COL32(75, 75, 75, 255);
    const ImU32 SLOT_NML_COLOR = IM_COL32(240, 240, 150, 150);
    const ImU32 SLOT_ACT_COLOR = IM_COL32(255, 255, 100, 255);
    const float TITLE_COL_SCALE = 155.f;
    const float TITLE_COL_OFFSET = 100.f;

    // Reference to the parent's
    LabelWrapper& label;
    const std::vector<std::string>& node_order;
    std::string& selected_node_name;
    std::string& hovered_node_name;
    std::map<std::string, GuiNode>& gui_nodes;
    const ImVec2& scroll_pos;
    const bool& is_link_creating;
    const std::map<const std::type_info*, VarEditor>& var_editors;

    // Private status
    // [None]

    // Private methods
    ImU32 genTitleColor(const size_t idx) {
        const size_t denom = node_order.size();
        assert(denom != 0);
        const float v = float(idx) / float(denom - 1);
        // clang-format off
        float r = (v <= 0.25f) ? 1.f :
                  (v <= 0.50f) ? 1.f - (v - 0.25f) / 0.25f : 0.f;
        float g = (v <= 0.25f) ? v / 0.25f :
                  (v <= 0.75f) ? 1.f : 1.f - (v - 0.75f) / 0.25f;
        float b = (v <= 0.50f) ? 0.f :
                  (v <= 0.75f) ? (v - 0.5f) / 0.25f : 1.f;
        // clang-format on
        r = r * TITLE_COL_SCALE * 1.0f + TITLE_COL_OFFSET;
        g = g * TITLE_COL_SCALE * 0.5f + TITLE_COL_OFFSET;
        b = b * TITLE_COL_SCALE * 1.0f + TITLE_COL_OFFSET;
        return IM_COL32(int(r), int(g), int(b), 200);
    }

    void drawInOutTag(bool in) {
        ImVec4 col;
        std::string text;
        if (in) {
            col = ImVec4(.7f, .4f, .1f, 1.f);
            text = "  in  ";
        } else {
            col = ImVec4(.2f, .7f, .1f, 1.f);
            text = "in/out";
        }
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
        ImGui::Button(text.c_str());
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    void drawNodeContent(FaseCore* core, const std::string& node_name,
                         const Node& node, GuiNode& gui_node,
                         const size_t order_idx,
                         const GUIPreference& preference) {
        ImGui::BeginGroup();  // Lock horizontal position
        bool s_flag = node.func_repr == OutputFuncStr() ||
                      node.func_repr == InputFuncStr();
        if (node.func_repr == InputFuncStr()) {
            ImGui::Text("Input");
        } else if (node.func_repr == OutputFuncStr()) {
            ImGui::Text("Output");
        } else {
            ImGui::Text("%zu) [%s] %s", order_idx, node.func_repr.c_str(),
                        node_name.c_str());
        }
        ImGui::Dummy(ImVec2(0.f, NODE_WINDOW_PADDING.y));

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

            // start to draw one argument
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));

            // draw input or inout.
            ImGui::SameLine();
            drawInOutTag(function.is_input_args[arg_idx]);

            // draw default argument editing
            ImGui::SameLine();
            if (!node.links[arg_idx].node_name.empty()) {
                // Link exists
                ImGui::Text("%s", arg_name.c_str());
            } else if (var_editors.count(arg_type)) {
                // Call registered GUI for editing
                auto& func = var_editors.at(arg_type);
                const Variable& var = node.arg_values[arg_idx];
                std::string expr;
                const std::string view_label = arg_name + "##" + node_name;
                Variable v = func(label(view_label), var);
                if (v) {
                    // Update argument
                    core->setNodeArg(node_name, arg_idx, "", v);
                }
            } else {
                // No GUI for editing
                if (s_flag) {
                    ImGui::Text("%s : Unset", arg_name.c_str());
                } else {
                    ImGui::Text("%s [default:%s]", arg_name.c_str(),
                                arg_repr.c_str());
                }
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
        }
        if (!s_flag) {
            int priority = node.priority;
            ImGui::SliderInt(label("priority"), &priority,
                             preference.priority_min, preference.priority_max);
            priority = std::min(priority, preference.priority_max);
            priority = std::max(priority, preference.priority_min);
            if (priority != node.priority) {
                core->setPriority(node_name, priority);
            }
        }
        ImGui::EndGroup();
    }

    void drawNodeBox(const ImVec2& node_rect_min, const ImVec2& node_size,
                     const bool is_active, const size_t order_idx) {
        const ImVec2 node_rect_max = node_rect_min + node_size;
        ImGui::InvisibleButton(label("node"), node_size);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        // Background
        const ImU32 bg_col = is_active ? BG_ACT_COLOR : BG_NML_COLOR;
        draw_list->AddRectFilled(node_rect_min, node_rect_max, bg_col, 4.f);
        // Border
        draw_list->AddRect(node_rect_min, node_rect_max, BORDER_COLOR, 4.f);
        // Title
        const float line_height = ImGui::GetTextLineHeight();
        const float pad_height = NODE_WINDOW_PADDING.y * 2;
        const ImVec2 node_title_rect_max =
                node_rect_min + ImVec2(node_size.x, line_height + pad_height);
        draw_list->AddRectFilled(node_rect_min, node_title_rect_max,
                                 genTitleColor(order_idx), 4.f);
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
             std::map<std::string, GuiNode>& gui_nodes, bool& is_link_creating)
        : hovered_slot_name(hovered_slot_name),
          hovered_slot_idx(hovered_slot_idx),
          is_hovered_slot_input(is_hovered_slot_input),
          gui_nodes(gui_nodes),
          is_link_creating(is_link_creating) {}

    void draw(FaseCore* core, std::function<void()>& updater) {
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
                    updater();
                }
                is_link_creating = false;
            }
        } else {
            if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0, 1.f) &&
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
    // const float ARROW_BEZIER_SIZE = 80.f;
    const float ARROW_HEAD_SIZE = 15.f;
    const float ARROW_HEAD_X_OFFSET = -ARROW_HEAD_SIZE * std::sqrt(3.f) * 0.5f;

    // Reference to the parent's
    std::string& hovered_slot_name;
    size_t& hovered_slot_idx;
    bool& is_hovered_slot_input;
    std::map<std::string, GuiNode>& gui_nodes;
    bool& is_link_creating;

    // Private status
    std::string hovered_slot_name_prev;
    size_t hovered_slot_idx_prev;
    bool is_hovered_slot_input_prev;

    // Private methods
    void drawLink(const ImVec2& s_pos, const ImVec2& d_pos) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float x_d = std::abs(d_pos.x - s_pos.x) * .7f;
        if (d_pos.x < s_pos.x) {
            x_d = std::pow(s_pos.x - d_pos.x, .7f) * 3.f;
        }
        float y_d = (d_pos.y - s_pos.y) * .2f;
        const ImVec2 s_pos_2 = s_pos + ImVec2(x_d, y_d);
        const ImVec2 d_pos_2 = d_pos - ImVec2(x_d, y_d);
        draw_list->AddBezierCurve(s_pos, s_pos_2, d_pos_2,
                                  d_pos - ImVec2(ARROW_HEAD_SIZE * 0.8f, 0),
                                  LINK_COLOR, 3.0f);
        // Arrow's triangle
        const ImVec2 t_pos_1 =
                d_pos + ImVec2(ARROW_HEAD_X_OFFSET, ARROW_HEAD_SIZE * 0.5f);
        const ImVec2 t_pos_2 =
                d_pos + ImVec2(ARROW_HEAD_X_OFFSET, -ARROW_HEAD_SIZE * 0.5f);
        draw_list->AddTriangleFilled(d_pos, t_pos_1, t_pos_2, LINK_COLOR);
    }
};

class ContextMenuGUI {
public:
    ContextMenuGUI(LabelWrapper& label, std::string& selected_node_name,
                   std::string& hovered_node_name,
                   std::string& hovered_slot_name,
                   const size_t& hovered_slot_idx,
                   const bool& is_hovered_slot_input, bool& request_add_node)
        : label(label),
          selected_node_name(selected_node_name),
          hovered_node_name(hovered_node_name),
          hovered_slot_name(hovered_slot_name),
          hovered_slot_idx(hovered_slot_idx),
          is_hovered_slot_input(is_hovered_slot_input),
          request_add_node(request_add_node) {}

    void draw(FaseCore* core, const std::function<void()>& updater) {
        // Right click
        if (ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows) &&
            ImGui::IsMouseClicked(1)) {
            if (!hovered_slot_name.empty()) {
                // Open pop up window for a slot
                selected_node_name = hovered_slot_name;
                selected_slot_name = hovered_slot_name;
                selected_slot_idx = hovered_slot_idx;
                is_selected_slot_input = is_hovered_slot_input;
                ImGui::OpenPopup(label("Popup: Slot context menu"));
            } else if (!hovered_node_name.empty()) {
                // Open pop up window for a node
                selected_node_name = hovered_node_name;
                ImGui::OpenPopup(label("Popup: Node context menu"));
            } else {
                // Open pop up window
                ImGui::OpenPopup(label("Popup: Common context menu"));
            }
        }

        // Slot menu
        if (ImGui::BeginPopup(label("Popup: Slot context menu"))) {
            ImGui::Text("Link \"%s[%d][%s]\"", selected_slot_name.c_str(),
                        int(selected_slot_idx),
                        is_selected_slot_input ? "in" : "out");
            ImGui::Separator();
            if (ImGui::MenuItem(label("Clear"))) {
                if (is_selected_slot_input) {
                    // Remove input link
                    core->delLink(selected_slot_name, selected_slot_idx);
                } else {
                    // Remove output links
                    std::vector<std::pair<std::string, size_t>> slots;
                    FindConnectedOutSlots(core->getNodes(), selected_node_name,
                                          selected_slot_idx, slots);
                    for (auto& it : slots) {
                        core->delLink(it.first, it.second);
                    }
                }
            }
            ImGui::EndPopup();
        }

        // Node menu
        if (ImGui::BeginPopup(label("Popup: Node context menu"))) {
            ImGui::Text("Node \"%s\"", selected_node_name.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem(label("Delete"))) {
                // TODO fix segfo bug
                core->delNode(selected_node_name);
                updater();
            }
            // TODO
            bool rename = false;
            if (ImGui::MenuItem(label("Rename"))) {
                rename = true;
            }
            ImGui::EndPopup();
            if (rename) {
                ImGui::OpenPopup(label("Popup: Rename node"));
            }
        }
        renamePopUp("Popup: Rename node", core, updater);

        // Node menu
        if (ImGui::BeginPopup(label("Popup: Common context menu"))) {
            if (ImGui::MenuItem(label("Add node"))) {
                // Call for another class `NodeAddingGUI`
                request_add_node = true;
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
    bool& request_add_node;

    // Private status
    std::string selected_slot_name;
    size_t selected_slot_idx = 0;
    bool is_selected_slot_input = false;

    char new_node_name[64];

    void renamePopUp(const char* popup_name, FaseCore* core,
                     const std::function<void()>& updater) {
        bool opened = true;
        if (ImGui::BeginPopupModal(label(popup_name), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            ImGui::InputText(label("New node name (ID)"), new_node_name,
                             sizeof(new_node_name));

            if (ImGui::Button("OK")) {
                core->renameNode(selected_node_name, new_node_name);
                updater();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
};

class PreferenceMenuGUI {
public:
    PreferenceMenuGUI(LabelWrapper& label, GUIPreference& preference)
        : label(label), preference(preference) {}

    void draw() {
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
    }

private:
    LabelWrapper& label;
    GUIPreference& preference;

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

class ReportWindow {
public:
    ReportWindow(LabelWrapper& label) : label(label) {}

    void draw(const std::map<std::string, ResultReport>& report_box) {
        if (report_box.empty()) {
            return;
        }

        ImGui::Begin("Report", NULL, ImGuiWindowFlags_MenuBar);

        ImGui::BeginMenuBar();

        if (ImGui::MenuItem(label("Nodes"), NULL, view == View::Nodes)) {
            view = View::Nodes;
        }
        if (ImGui::MenuItem(label("Steps"), NULL, view == View::Steps)) {
            view = View::Steps;
        }

        ImGui::EndMenuBar();

        // for sort rule
        ImGui::Text("Sort rule : ");
        ImGui::SameLine();
        if (ImGui::RadioButton("Name", sort_rule == SortRule::Name)) {
            sort_rule = SortRule::Name;
        };
        ImGui::SameLine();
        if (ImGui::RadioButton("Time", sort_rule == SortRule::Time)) {
            sort_rule = SortRule::Time;
        }

        // apply view filter
        std::map<std::string, ResultReport> reports =
                SubGroup(report_box, getFilter());

        // sort
        auto keys = GetKeys(reports);
        std::sort(std::begin(keys), std::end(keys), getSorter(reports));

        const float vmaxf = getTotalTime(reports);

        // view
        for (auto& key : keys) {
            float v = getSec(reports.at(key));
            if (key == TotalTimeStr()) {
                ImGui::Text("total :  %.3f msec", v * 1e3f);
                continue;
            }
            ImGui::ProgressBar(
                    v / vmaxf,
                    ImVec2(ImGui::GetContentRegionAvailWidth() * 0.5f, 0));
            ImGui::SameLine();
            ImGui::Text("%s", key.c_str());
            ImGui::SameLine();
            ImGui::Text(" : %.3f msec", v * 1e3f);
        }

        ImGui::End();
    }

private:
    enum class View {
        Nodes,
        Steps,
    };
    enum class SortRule : int {
        Name,
        Time,
    };
    LabelWrapper& label;
    View view = View::Nodes;
    SortRule sort_rule = SortRule::Name;

    std::function<bool(const std::pair<std::string, ResultReport>&)>
    getFilter() {
        if (view == View::Nodes) {
            return [](const auto& v) {
                return std::get<0>(v).find(ReportHeaderStr()) ==
                       std::string::npos;
            };
        } else if (view == View::Steps) {
            return [](const auto& v) {
                return std::get<0>(v).find(ReportHeaderStr()) !=
                       std::string::npos;
            };
        }
        return [](const auto&) { return false; };
    }

    std::function<bool(const std::string&, const std::string&)> getSorter(
            std::map<std::string, ResultReport>& reports) {
        if (sort_rule == SortRule::Name) {
            return [](const auto& a, const auto& b) { return a < b; };
        } else if (sort_rule == SortRule::Time) {
            return [&reports](const auto& a, const auto& b) {
                return reports.at(a).execution_time >
                       reports.at(b).execution_time;
            };
        }
        return [](const auto&, const auto&) { return false; };
    }

    float getTotalTime(const std::map<std::string, ResultReport>& reports) {
        if (view == View::Nodes) {
            float total = 0.f;
            for (const auto& report : reports) {
                total += getSec(std::get<1>(report));
            }
            return total;
        } else if (view == View::Steps) {
            return getSec(reports.at(TotalTimeStr()));
        }
        return 0.f;
    }

    float getSec(const ResultReport& repo) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                       repo.execution_time)
                       .count() *
               1e-6f;
    }
};

// GUI for <bool>
bool ImGuiInputValue(const char* label, bool* v) {
    return ImGui::Checkbox(label, v);
}

// GUI for <int>
bool ImGuiInputValue(const char* label, int* v,
                     int v_min = std::numeric_limits<int>::min(),
                     int v_max = std::numeric_limits<int>::max()) {
    const float v_speed = 1.0f;
    return ImGui::DragInt(label, v, v_speed, v_min, v_max);
}

// GUI for <float>
bool ImGuiInputValue(const char* label, float* v,
                     float v_min = std::numeric_limits<float>::min(),
                     float v_max = std::numeric_limits<float>::max()) {
    const float v_speed = 0.1f;
    return ImGui::DragFloat(label, v, v_speed, v_min, v_max, "%.4f");
}

// GUI for <int link type>
template <typename T, typename C = int>
bool ImGuiInputValue(const char* label, T* v) {
    C i = static_cast<C>(*v);
    const bool ret = ImGuiInputValue(label, &i, std::numeric_limits<C>::min(),
                                     std::numeric_limits<C>::max());
    *v = static_cast<T>(i);
    return ret;
}

// GUI for <double>
bool ImGuiInputValue(const char* label, double* v) {
    return ImGuiInputValue<double, float>(label, v);
}

// GUI for <std::string>
bool ImGuiInputValue(const char* label, std::string* v) {
    // Copy to editing buffer
    char str_buf[1024];
    const size_t n_str = sizeof(str_buf);
    strncpy(str_buf, (*v).c_str(), n_str);
    // Show GUI
    const bool ret = ImGui::InputText(label, str_buf, n_str);
    // Back to the value
    (*v) = str_buf;
    return ret;
}

template <typename T>
void EditorMaker(std::map<const std::type_info*, VarEditor>& var_editors) {
    var_editors[&typeid(T)] = [](const char* label, const Variable& a) {
        T copy = *a.getReader<T>();
        if (ImGuiInputValue(label, &copy)) {
            return Variable(std::move(copy));
        }
        return Variable();
    };
}

void SetUpVarEditors(std::map<const std::type_info*, VarEditor>* var_editors) {
    EditorMaker<int>(*var_editors);

    EditorMaker<bool>(*var_editors);
    // * Character types
    EditorMaker<char>(*var_editors);
    EditorMaker<unsigned char>(*var_editors);
    // * Integer types
    EditorMaker<int>(*var_editors);
    EditorMaker<unsigned int>(*var_editors);
    EditorMaker<short>(*var_editors);
    EditorMaker<unsigned short>(*var_editors);
    EditorMaker<long>(*var_editors);
    EditorMaker<unsigned long>(*var_editors);
    EditorMaker<long long>(*var_editors);
    EditorMaker<unsigned long long>(*var_editors);
    // * Floating types
    EditorMaker<float>(*var_editors);
    EditorMaker<double>(*var_editors);

    // STD containers
    // * string
    EditorMaker<std::string>(*var_editors);
}

}  // anonymous namespace

#if 1
class GUIEditor::Impl {
public:
    Impl() {}

    bool run(FaseCore* core, const TypeUtils& utils,
             const std::string& win_title, const std::string& label_suffix);

    bool addVarEditor(const std::type_info* p, VarEditor&& f);

private:
    View view;

    std::map<std::string, ResultReport> reports;
    std::map<const std::type_info*, VarEditor> var_editors;
};

bool GUIEditor::Impl::run(FaseCore* core, const TypeUtils& utils,
                          const std::string& win_title,
                          const std::string& label_suffix) {
    const std::vector<Issue> issues = view.draw(*core, win_title, label_suffix);

    for (const Issue& issue : issues) {
        // TODO
    }

    return true;
}

#else
class GUIEditor::Impl {
public:
    Impl()
        // Module dependencies are written here
        : node_adding_gui(label, request_add_node),
          run_pipeline_gui(label, is_pipeline_updated, &reports),
          native_code_gui(label),
          layout_optimize_gui(label, gui_nodes),
          save_gui(label),
          load_gui(label),
          node_list_gui(label, node_order, selected_node_name,
                        hovered_node_name),
          links_gui(hovered_slot_name, hovered_slot_idx, is_hovered_slot_input,
                    gui_nodes, is_link_creating),
          node_boxes_gui(label, node_order, selected_node_name,
                         hovered_node_name, gui_nodes, scroll_pos,
                         is_link_creating, var_editors),
          context_menu_gui(label, selected_node_name, hovered_node_name,
                           hovered_slot_name, hovered_slot_idx,
                           is_hovered_slot_input, request_add_node),
          preference_menu_gui(label, preference),
          add_inoutput_menu_gui(label),
          report_window(label) {
        var_editors = std::map<const std::type_info*, VarEditor>();
        SetUpVarEditors(&var_editors);
    }

    bool run(FaseCore* core, const TypeUtils& utils,
             const std::string& win_title, const std::string& label_suffix);

    bool addVarEditor(const std::type_info* p, VarEditor&& f);

private:
    // GUI components
    NodeAddingGUI node_adding_gui;
    RunPipelineGUI run_pipeline_gui;
    NativeCodeGUI native_code_gui;
    LayoutOptimizeGUI layout_optimize_gui;
    SaveGUI save_gui;
    LoadGUI load_gui;
    NodeListGUI node_list_gui;
    LinksGUI links_gui;
    NodeBoxesGUI node_boxes_gui;
    ContextMenuGUI context_menu_gui;
    PreferenceMenuGUI preference_menu_gui;
    AddInOutputGUI add_inoutput_menu_gui;

    ReportWindow report_window;

    // Common status
    LabelWrapper label;  // Label wrapper for suffix to generate unique label
    GUIPreference preference;
    std::vector<std::string> node_order;
    std::string selected_node_name;
    std::string hovered_node_name;
    std::string hovered_slot_name;
    size_t hovered_slot_idx;
    bool is_hovered_slot_input;
    std::map<std::string, GuiNode> gui_nodes;
    ImVec2 scroll_pos = ImVec2(0.0f, 0.0f);
    bool is_link_creating = false;
    bool is_pipeline_updated = false;
    std::string prev_nodes_str;
    bool request_add_node = false;
    std::map<std::string, ResultReport> reports;
    std::map<const std::type_info*, VarEditor> var_editors;

    void updateGuiNodes(FaseCore* core) {
        // Clear cache
        is_pipeline_updated = false;

        const std::map<std::string, Node>& nodes = core->getNodes();
        std::string nodes_str = CoreToString(*core, {});
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const size_t n_args = it->second.links.size();
            // Check GUI node existence
            if (!gui_nodes.count(node_name) ||
                gui_nodes[node_name].arg_size() != n_args) {
                // Create new node and allocate for link slots
                gui_nodes[node_name].alloc(n_args);
                is_pipeline_updated = true;
            }
        }
        // Detect changes by the numbers of nodes and links
        if (nodes_str != prev_nodes_str) {
            is_pipeline_updated = true;
            prev_nodes_str = std::move(nodes_str);
            // Remove unused GUI nodes
            while (true) {
                bool ok_f = true;
                for (auto it = gui_nodes.begin(); it != gui_nodes.end(); it++) {
                    if (nodes.count(it->first) == 0) {
                        gui_nodes.erase(it);
                        ok_f = false;
                        break;
                    }
                }
                if (ok_f) {
                    break;
                }
            }
            // Update node order
            node_order.clear();
            for (auto&& l : GetCallOrder(core->getNodes())) {
                node_order.insert(std::end(node_order), std::begin(l),
                                  std::end(l));
            }
        }
    }
};

bool GUIEditor::Impl::run(FaseCore* core, const TypeUtils& type_utils,
                          const std::string& win_title,
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

    std::function<void()> updater = [this, core]() { updateGuiNodes(core); };

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        preference_menu_gui.draw();
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to add new node
        node_adding_gui.draw(core, updater);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to run
        run_pipeline_gui.draw(core);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to show native code
        native_code_gui.draw(core, type_utils);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing
        // Menu to optimize the node layout
        layout_optimize_gui.draw(core, preference.auto_layout);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing

        add_inoutput_menu_gui.draw(core, updater);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing

        save_gui.draw(core, type_utils);
        ImGui::Dummy(ImVec2(5, 0));  // Spacing

        load_gui.draw(core, type_utils, updater);

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

    ImGui::End();  // End window

    report_window.draw(reports);

    return true;
}
#endif

bool GUIEditor::Impl::addVarEditor(const std::type_info* p, VarEditor&& f) {
    var_editors[p] = std::forward<VarEditor>(f);
    return true;
}

// ------------------------------- pImpl pattern -------------------------------
GUIEditor::GUIEditor() : impl(new GUIEditor::Impl()) {}
bool GUIEditor::addVarEditor(const std::type_info* p, VarEditor&& f) {
    return impl->addVarEditor(p, std::forward<VarEditor>(f));
}
GUIEditor::~GUIEditor() {}
bool GUIEditor::run(FaseCore* core, const TypeUtils& utils,
                    const std::string& win_title,
                    const std::string& label_suffix) {
    return impl->run(core, utils, win_title, label_suffix);
}

}  // namespace fase
