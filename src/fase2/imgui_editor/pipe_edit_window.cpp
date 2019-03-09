
#include "pipe_edit_window.h"

#include "../constants.h"

#include <cmath>

namespace fase {

using std::map, std::string, std::vector;
using size_t = std::size_t;

namespace {

PopupRAII BeginPopupContext(const char* str, bool condition, int button) {
    if (condition && ImGui::IsWindowHovered() && ImGui::IsMouseReleased(button))
        ImGui::OpenPopup(str);
    return {str};
}

PopupModalRAII BeginPopupModal(
        const char* str, bool condition,
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize) {
    if (condition) ImGui::OpenPopup(str);
    return {str, true, flags};
}

bool IsSpecialNodeName(const string& name) {
    return name == InputNodeName() || name == OutputNodeName();
}

float GetVolume(const size_t& idx) {
    if (idx == 0)
        return 0.f;
    else if (idx == 1)
        return 1.f;
    else {
        size_t i = size_t(std::pow(2, int(std::log2(idx - 1))));
        return ((idx - i) * 2 - 1) / 2.f / float(i);
    }
}

constexpr float NODE_COL_SCALE = 155.f;
constexpr float NODE_COL_OFFSET = 100.f;

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

const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 6.f);

const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
const ImU32 BORDER_ACT_COLOR = IM_COL32(30, 30, 255, 255);
const ImU32 BG_NML_COLOR = IM_COL32(40, 40, 40, 200);

bool drawNodeBox(const ImVec2& node_rect_min, const ImVec2& node_size,
                 const bool is_active, const size_t node_id,
                 LabelWrapper label) {
    const ImVec2 node_rect_max = node_rect_min + node_size;
    bool ret = ImGui::InvisibleButton(label("node"), node_size);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Background
    draw_list->AddRectFilled(node_rect_min, node_rect_max, BG_NML_COLOR, 4.f);
    // Title
    const float line_height = ImGui::GetTextLineHeight();
    const float pad_height = NODE_WINDOW_PADDING.y * 2;
    const ImVec2 node_title_rect_max =
            node_rect_min + ImVec2(node_size.x, line_height + pad_height);
    draw_list->AddRectFilled(node_rect_min, node_title_rect_max,
                             GenNodeColor(node_id), 4.f);
    // Border
    const ImU32 border_col = is_active ? BORDER_ACT_COLOR : BORDER_COLOR;
    const float thickness = is_active ? 5.f : 1.f;
    draw_list->AddRect(node_rect_min, node_rect_max, border_col, 4.f,
                       ImDrawCornerFlags_All, thickness);
    return ret;
}

bool IsSpecialFuncName(const string& n_name) {
    return n_name == InputNodeName() || n_name == OutputNodeName();
}

void DrawColTextBox(const ImVec4& col, const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    ImGui::Button(text);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

void DrawInOutTag(bool in, bool simple = false) {
    ImVec4 col;
    std::string text;
    if (in) {
        col = ImVec4(.7f, .4f, .1f, 1.f);
        if (simple) {
            text = " ";
        } else {
            text = "  in  ";
        }
    } else {
        col = ImVec4(.2f, .7f, .1f, 1.f);
        if (simple) {
            text = " ";
        } else {
            text = "in/out";
        }
    }
    DrawColTextBox(col, text.c_str());
}

constexpr float SLOT_RADIUS = 4.f;
constexpr float SLOT_SPACING = 3.f;

void drawNodeContent(const string& n_name, const Node& node,
                     const FunctionUtils& func_utils, const string& p_name,
                     GuiNode& gui_node, bool simple, LabelWrapper label,
                     VarEditors* var_editors, Issues* issues) {
    label.addSuffix("##" + n_name);
    ImGui::BeginGroup();  // Lock horizontal position
    bool s_flag = IsSpecialFuncName(n_name);

    ImGui::Text("[%s] %s", node.func_name.c_str(), n_name.c_str());

    ImGui::Dummy(ImVec2(0.f, NODE_WINDOW_PADDING.y));

    const size_t n_args = node.args.size();
    assert(gui_node.arg_poses.size() == n_args);

    // Draw arguments
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        // Save argument position
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        gui_node.arg_poses[arg_idx] =
                ImVec2(pos.x - NODE_WINDOW_PADDING.x,
                       pos.y + ImGui::GetTextLineHeight() * 0.5f);
        // Fetch from structures
        const std::string& arg_name = func_utils.arg_names[arg_idx];
        const std::type_index& arg_type = node.args[arg_idx].getType();

        // start to draw one argument
        ImGui::Dummy(ImVec2(SLOT_SPACING, 0));

        // draw input or inout.
        ImGui::SameLine();
        DrawInOutTag(func_utils.is_input_args[arg_idx], simple);

        // draw default argument editing
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (simple) {
            ImGui::Text("%s", arg_name.c_str());
        } else if (var_editors->count(arg_type)) {
            // Call registered GUI for editing
            auto& func = var_editors->at(arg_type);
            const Variable& var = node.args[arg_idx];
            std::string expr;
            const std::string view_label = arg_name + "##" + n_name;
            Variable v = func(label(view_label), var);
            if (v) {
                struct {
                    Variable v;
                    string n_name;
                    string p_name;
                    size_t arg_idx;
                    void operator()(CoreManager* pcm) {
                        (*pcm)[p_name].setArgument(n_name, arg_idx, v);
                    }
                } a{std::move(v), n_name, p_name, arg_idx};
                issues->emplace_back(a);
            }
        } else {
            // No GUI for editing
            ImGui::Text("%s", arg_name.c_str());
        }
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
    }
    if (!s_flag && !simple) {
        int priority = node.priority;
        ImGui::PushItemWidth(100);
        ImGui::InputInt(label("priority"), &priority);
        ImGui::PopItemWidth();
        priority = std::min(priority, std::numeric_limits<int>::max() - 1);
        priority = std::max(priority, std::numeric_limits<int>::min() + 1);
        if (priority != node.priority) {
            issues->emplace_back([priority, p_name, n_name](auto pcm) {
                (*pcm)[p_name].setPriority(n_name, priority);
            });
        }
    }
    ImGui::EndGroup();
}

constexpr ImU32 SLOT_ACT_COLOR = IM_COL32(255, 255, 100, 255);

void drawLinkSlots(const GuiNode& gui_node) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const size_t n_args = gui_node.arg_poses.size();
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        // Get slot position
        const ImVec2 inp_slot = gui_node.getInputSlot(arg_idx) +
                                ImVec2(LinksView::SLOT_HOVER_RADIUS, 0);
        const ImVec2 out_slot = gui_node.getOutputSlot(arg_idx) -
                                ImVec2(LinksView::SLOT_HOVER_RADIUS, 0);
        // Get hovered conditions
        const char& inp_hov = gui_node.arg_inp_hovered[arg_idx];
        const char& out_hov = gui_node.arg_out_hovered[arg_idx];
        // Draw
        const ImU32 SLOT_NML_COLOR = GenNodeColor(gui_node.id);
        const ImU32 inp_col = inp_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
        const ImU32 out_col = out_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
        if (gui_node.id != 0) {
            draw_list->AddCircleFilled(inp_slot, SLOT_RADIUS, inp_col);
        }
        if (gui_node.id != 1) {
            draw_list->AddCircleFilled(out_slot, SLOT_RADIUS, out_col);
        }
    }
}

size_t SearchUnusedID(const map<string, GuiNode>& node_gui_utils) {
    for (size_t i = 0;; i++) {
        bool f = true;
        for (auto& [_, g_node] : node_gui_utils) {
            if (i == g_node.id) {
                f = false;
                break;
            }
        }
        if (f) return i;
    }
}

}  // namespace

ImU32 GenNodeColor(const size_t& idx) {
    const float v = GetVolume(idx);
    // clang-format off
    float r = (v <= 0.25f) ? 1.f :
              (v <= 0.50f) ? 1.f - (v - 0.25f) / 0.25f : 0.f;
    float g = (v <= 0.25f) ? v / 0.25f :
              (v <= 0.75f) ? 1.f : 1.f - (v - 0.75f) / 0.25f;
    float b = (v <= 0.50f) ? 0.f :
              (v <= 0.75f) ? (v - 0.5f) / 0.25f : 1.f;
    // clang-format on
    r = r * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    g = g * NODE_COL_SCALE * 0.5f + NODE_COL_OFFSET;
    b = b * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    return IM_COL32(int(r), int(g), int(b), 200);
}

string EditWindow::drawNodes(const PipelineAPI& core_api, LabelWrapper label,
                             const int base_channel, Issues* issues,
                             VarEditors* var_editors) {
    const ImVec2 canvas_offset = ImGui::GetCursorScreenPos();
    // Draw node box
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    string hovered = "";
    for (auto& [n_name, node] : core_api.getNodes()) {
        GuiNode& gui_node = node_gui_utils[n_name];
        ImGui::PushID(label(n_name));

        // Draw node contents first
        draw_list->ChannelsSetCurrent(1 + base_channel);  // Foreground
        ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos +
                                  NODE_WINDOW_PADDING);
        drawNodeContent(n_name, node, funcs.at(node.func_name), pipe_name,
                        gui_node, false, label, var_editors, issues);

        // Fit to content size
        gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

        // Draw node box
        draw_list->ChannelsSetCurrent(0 + base_channel);  // Background
        ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos);
        if (drawNodeBox(canvas_offset + gui_node.pos, gui_node.size,
                        selected_node_name == n_name, gui_node.id, label)) {
            selected_node_name = n_name;
        }

        // Draw link slots
        drawLinkSlots(gui_node);

        if (ImGui::IsItemHovered()) {
            hovered = n_name;
        }
        ImGui::PopID();
    }
    return hovered;
}

void EditWindow::drawNodeContextMenu(const string& hovered,
                                     const PipelineAPI& core_api,
                                     LabelWrapper label, Issues* issues) {
    label.addSuffix("##NContext");
    bool rename_open_f = false;
    bool allocate_function_f = false;
    bool edit_input_output_f = false;

    for (auto& [n_name, node] : core_api.getNodes()) {
        if (auto p_raii = BeginPopupContext(label(n_name + "_context"),
                                            hovered == n_name, 1)) {
            selected_node_name = n_name;
            ImGui::Text("%s", n_name.c_str());
            ImGui::Separator();
            if (ImGui::Selectable("delete")) {
                issues->emplace_back(
                        [p_name = pipe_name, n_name = n_name](auto pcm) {
                            (*pcm)[p_name].delNode(n_name);
                        });
            }
            if (ImGui::Selectable("rename")) {
                rename_open_f = true;
            }
            ImGui::Separator();
            if (IsSpecialNodeName(n_name)) {
                if (ImGui::Selectable(label("edit input/output"))) {
                    edit_input_output_f = true;
                }
            } else {
                if (ImGui::Selectable(label("alocate function"))) {
                    allocate_function_f = true;
                }
            }
        }
    }

    if (auto raii = BeginPopupModal(label("rename_modal"), rename_open_f)) {
        if (new_node_name_it.draw(label("new name"))) {
            issues->emplace_back([p_name = pipe_name,
                                  n_name = new_node_name_it.text(),
                                  old = selected_node_name](auto pcm) {
                (*pcm)[p_name].renameNode(old, n_name);
            });
            ImGui::CloseCurrentPopup();
        }
    }

    if (auto raii = BeginPopupModal(label("allocate function modal"),
                                    allocate_function_f)) {
        function_combo.draw(label("func_combo"));
        if (ImGui::Button(label("OK"))) {
            issues->emplace_back([p_name = pipe_name,
                                  f_name = function_combo.text(),
                                  n_name = selected_node_name](auto pcm) {
                (*pcm)[p_name].allocateFunc(f_name, n_name);
            });
            ImGui::CloseCurrentPopup();
        }
    }

    if (auto raii = BeginPopupModal(label("Edit input/output"),
                                    edit_input_output_f)) {
        ImGui::BeginGroup();
        int size = input_arg_name_its.size();
        if (ImGui::InputInt(label("input size"), &size)) {
            input_arg_name_its.resize(size_t(size));
        }
        int i = 0;
        for (auto& it : input_arg_name_its) {
            it.draw(label("input" + std::to_string(i++)));
        }
        if (ImGui::Button(label("OK##ine"))) {
            vector<string> arg_names;
            for (auto& it : input_arg_name_its) {
                arg_names.emplace_back(it.text());
            }
            issues->emplace_back([arg_names = std::move(arg_names),
                                  p_name = pipe_name](auto pcm) {
                (*pcm)[p_name].supposeInput(arg_names);
            });
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        size = output_arg_name_its.size();
        if (ImGui::InputInt(label("output size"), &size)) {
            output_arg_name_its.resize(size_t(size));
        }
        i = 0;
        for (auto& it : output_arg_name_its) {
            it.draw(label("output" + std::to_string(i++)));
        }
        if (ImGui::Button(label("OK##oute"))) {
            vector<string> arg_names;
            for (auto& it : output_arg_name_its) {
                arg_names.emplace_back(it.text());
            }
            issues->emplace_back([arg_names = std::move(arg_names),
                                  p_name = pipe_name](auto pcm) {
                (*pcm)[p_name].supposeOutput(arg_names);
            });
        }
        ImGui::EndGroup();
    }
}

void EditWindow::drawCanvasContextMenu(const string& hovered,
                                       LabelWrapper label, Issues* issues) {
    label.addSuffix("##CContext");
    bool modal_f = false;
    if (auto raii = BeginPopupContext(label("canvas"), hovered.empty(), 1)) {
        if (ImGui::Selectable(label("new node"))) {
            modal_f = true;
        }
        ImGui::Separator();
        if (ImGui::Selectable(label("run"))) {
            issues->emplace_back(
                    [this](auto pcm) { (*pcm)[pipe_name].run(&report); });
        }
    }

    if (auto raii = BeginPopupModal(label("name_modal"), modal_f)) {
        if (new_node_name_it.draw(label("new name"))) {
            issues->emplace_back([p_name = pipe_name,
                                  n_name = new_node_name_it.text()](auto pcm) {
                (*pcm)[p_name].newNode(n_name);
            });
            ImGui::CloseCurrentPopup();
        }
    }
}

void EditWindow::drawCanvasPannel(const PipelineAPI& core_api,
                                  LabelWrapper label, Issues* issues,
                                  VarEditors* var_editors) {
    label.addSuffix("##Canvas");
    ChildRAII raii(label(""));

    DrawCanvas(ImVec2(0, 0), 100);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(3);
    string hovered = drawNodes(core_api, label, 1, issues, var_editors);

    draw_list->ChannelsSetCurrent(0);
    links_view.draw(core_api.getNodes(), core_api.getLinks(), pipe_name,
                    node_gui_utils, issues);
    draw_list->ChannelsMerge();

    drawNodeContextMenu(hovered, core_api, label, issues);
    drawCanvasContextMenu(hovered, label, issues);
}

void EditWindow::updateGuiNodeUtils(const PipelineAPI& core_api) {
    auto& nodes = core_api.getNodes();

    // delete unused gui nodes.
    vector<string> erase_list;
    for (auto& [gn_name, node] : node_gui_utils) {
        if (!core_api.getNodes().count(gn_name)) {
            erase_list.emplace_back(gn_name);
            continue;
        }
        node_gui_utils[gn_name].alloc(nodes.at(gn_name).args.size());
    }
    for (auto& gn_name : erase_list) {
        node_gui_utils.erase(gn_name);
        if (selected_node_name == gn_name) {
            selected_node_name = "";
        }
    }

    // make new gui nodes.
    for (auto& [n_name, node] : nodes) {
        if (!node_gui_utils.count(n_name)) {
            node_gui_utils[n_name].id = SearchUnusedID(node_gui_utils);
            node_gui_utils[n_name].alloc(nodes.at(n_name).args.size());
        }
    }

    // set node positions
    static constexpr float MIN_POS_X = 30;
    static constexpr float MIN_POS_Y = 10;
    static constexpr float INTERVAL_X = 50;
    static constexpr float INTERVAL_Y = 10;

    auto order = GetRunOrder(nodes, core_api.getLinks());

    float baseline_y = 0;
    float x = MIN_POS_X, y = MIN_POS_Y, maxy = 0;
    for (auto& name_set : order) {
        float maxx = 0;
        for (auto& n_name : name_set) {
            maxx = std::max(maxx, node_gui_utils[n_name].size.x);
        }
        for (auto& n_name : name_set) {
            node_gui_utils[n_name].pos = ImVec2(x, y + baseline_y);
            y += node_gui_utils[n_name].size.y + INTERVAL_Y;
        }
        maxy = std::max(maxy, y);
        y = MIN_POS_Y;
        x += maxx + INTERVAL_X;
    }
}

void EditWindow::updateMembers(const CoreManager& cm, const string& p_name) {
    vector<string> f_names;
    funcs = cm.getFunctionUtils(p_name);
    f_names.reserve(funcs.size() + 1);
    for (auto& [f_name, f_util] : funcs) {
        if (f_name != p_name) {
            f_names.emplace_back(f_name);
        }
    }
    function_combo.set(std::move(f_names));

    updateGuiNodeUtils(cm[p_name]);

    pipe_name = p_name;
}

bool EditWindow::draw(const string& p_name, const string& win_title,
                      const CoreManager& cm, LabelWrapper label, Issues* issues,
                      VarEditors* var_editors) {
    label.addSuffix("##" + p_name);
    updateMembers(cm, p_name);

    bool opened = true;
    if (auto raii = WindowRAII(label(win_title + " : Pipe Edit - " + p_name),
                               &opened)) {
        drawCanvasPannel(cm[p_name], label, issues, var_editors);
    }

    return opened;
}

}  // namespace fase
