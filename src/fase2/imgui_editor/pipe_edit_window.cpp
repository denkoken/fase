
#include "pipe_edit_window.h"

#include "../constants.h"

#include <cmath>

namespace fase {

using std::map, std::string, std::vector;
using size_t = std::size_t;

namespace {

bool IsSpecialNodeName(const string& name) {
    return name.empty() || name == InputNodeName() || name == OutputNodeName();
}

bool IsSpecialFuncName(const string& name) {
    return name.empty() || name == kInputFuncName || name == kOutputFuncName;
}

string ToSnakeCase(const string& in) {
    string str = in;
    if (str[0] <= 'Z' && str[0] >= 'A') {
        str[0] -= 'A' - 'a';
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        replace({c}, "_" + string({char((c - 'A') + 'a')}), &str);
    }
    return str;
}

string GetEasyName(const string& func_name, const map<string, Node>& nodes) {
    string node_name = ToSnakeCase(func_name);

    int i = 0;
    while (nodes.count(node_name)) {
        node_name = ToSnakeCase(func_name) + std::to_string(++i);
    }
    return node_name;
}

const char* getDescription(const FunctionUtils& utils) {
    const static char dum[] = "No Description.";
    if (utils.description.empty()) {
        return dum;
    }
    return utils.description.c_str();
}

template <class Task>
void WrapError(Task&& task, string* str) {
    try {
        task();
    } catch (ErrorThrownByNode& e) {
        *str = e.what();
        try {
            e.rethrow_nested();
        } catch (std::exception& d) {
            *str += " : ";
            *str += d.what();
        } catch (...) {
        }
    }
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

ImVec4 VolumeColor(float v) {
    // clang-format off
    float r = (v <= 0.25f) ? 1.f :
              (v <= 0.50f) ? 1.f - (v - 0.25f) / 0.25f : 0.f;
    float g = (v <= 0.25f) ? v / 0.25f :
              (v <= 0.75f) ? 1.f : 1.f - (v - 0.75f) / 0.25f;
    float b = (v <= 0.50f) ? 0.f :
              (v <= 0.75f) ? (v - 0.5f) / 0.25f : 1.f;
    // clang-format on
    return {r, g, b, 1};
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
    ImGui::BeginGroup(); // Lock horizontal position
    bool s_flag = IsSpecialNodeName(n_name);

    if (s_flag) {
        ImGui::Text("%s", n_name.c_str());
    } else {
        ImGui::Text("[%s] %s", node.func_name.c_str(), n_name.c_str());
    }

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
            if (var) {
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
                ImGui::Text("[empty] %s", arg_name.c_str());
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

} // namespace

ImU32 GenNodeColor(const size_t& idx) {
    const float v = GetVolume(idx);
    ImVec4 col = VolumeColor(v);
    float r = col.x * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    float g = col.y * NODE_COL_SCALE * 0.5f + NODE_COL_OFFSET;
    float b = col.z * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    return IM_COL32(int(r), int(g), int(b), 200);
}

bool EditWindow::drawNormalNode(const string& n_name, const Node& node,
                                GuiNode& gui_node, const ImVec2& canvas_offset,
                                int base_channel, LabelWrapper& label,
                                Issues* issues, VarEditors* var_editors) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Draw node contents first
    draw_list->ChannelsSetCurrent(1 + base_channel); // Foreground
    ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos +
                              NODE_WINDOW_PADDING);
    drawNodeContent(n_name, node, funcs.at(node.func_name), pipe_name, gui_node,
                    false, label, var_editors, issues);

    // Fit to content size
    gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

    // Draw node box
    draw_list->ChannelsSetCurrent(0 + base_channel); // Background
    ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos);
    if (drawNodeBox(canvas_offset + gui_node.pos, gui_node.size,
                    selected_node_name == n_name, gui_node.id, label)) {
        selected_node_name = n_name;
    }

    // Draw link slots
    drawLinkSlots(gui_node);

    return ImGui::IsItemHovered();
}

bool EditWindow::drawSmallNode(const string& n_name, const Node& node,
                               GuiNode& gui_node, const ImVec2& canvas_offset,
                               int base_channel, LabelWrapper& label,
                               Issues* issues, VarEditors* var_editors) {
    constexpr int LINK_POS_INTERVAL = 16;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSetCurrent(0 + base_channel);

    // set node size
    gui_node.size = ImVec2(20, gui_node.arg_poses.size() * LINK_POS_INTERVAL) +
                    NODE_WINDOW_PADDING * 2;

    // compute argument slot positions
    ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos);
    for (size_t i = 0; i < gui_node.arg_poses.size(); i++) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        gui_node.arg_poses[i] =
                pos +
                ImVec2(-NODE_WINDOW_PADDING.x, LINK_POS_INTERVAL * (i + 0.3f)) +
                NODE_WINDOW_PADDING;
    }

    ImVec2 pos = canvas_offset + gui_node.pos;
    draw_list->AddRectFilled(pos, pos + gui_node.size, BG_NML_COLOR, 4.f);
    draw_list->AddRect(pos, pos + gui_node.size, GenNodeColor(gui_node.id), 4.f,
                       ImDrawCornerFlags_All, 3);
    if (ImGui::InvisibleButton(label(n_name + "node"), gui_node.size)) {
        selected_node_name = n_name;
        // ImGui::OpenPopup(label(n_name));
    }
    bool ret = ImGui::IsItemHovered();

    if (auto raii = PopupRAII(label(n_name))) {
        drawNodeContent(n_name, node, funcs.at(node.func_name), pipe_name,
                        gui_node, false, label, var_editors, issues);
        gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;
        // run this pipeline.
        if (GetIsKeyPressed('r', true)) {
            issues->emplace_back([this](auto pcm) {
                WrapError([&] { (*pcm)[pipe_name].run(&report); },
                          &err_message);
            });
        }
    }

    // Draw link slots
    drawLinkSlots(gui_node);
    return ret;
}

string EditWindow::drawNodes(const PipelineAPI& core_api, LabelWrapper label,
                             const int base_channel, Issues* issues,
                             VarEditors* var_editors) {
    const ImVec2 canvas_offset = ImGui::GetCursorScreenPos();
    // Draw node boxes
    string hovered = "";
    for (auto& [n_name, node] : core_api.getNodes()) {
        GuiNode& gui_node = node_gui_utils[n_name];
        ImGui::PushID(label(n_name));
        if (small_node_mode && selected_node_name != n_name) {
            if (drawSmallNode(n_name, node, gui_node, canvas_offset,
                              base_channel, label, issues, var_editors))
                hovered = n_name;
        } else {
            if (drawNormalNode(n_name, node, gui_node, canvas_offset,
                               base_channel, label, issues, var_editors))
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
    bool delete_f = false;

    for (auto& [n_name, node] : core_api.getNodes()) {
        if (auto p_raii = BeginPopupContext(label(n_name + "_context"),
                                            hovered == n_name, 1)) {
            selected_node_name = n_name;
            ImGui::Text("%s", n_name.c_str());
            ImGui::Separator();
            if (IsSpecialNodeName(n_name)) {
                edit_input_output_f =
                        ImGui::Selectable(label("edit input/output"));
            } else {
                delete_f = ImGui::Selectable("delete");
                rename_open_f = ImGui::Selectable("rename");
                ImGui::Separator();
                allocate_function_f =
                        ImGui::Selectable(label("alocate function"));
            }
        }
    }

    if (delete_f ||
        (GetIsKeyPressed(ImGuiKey_Backspace) && !selected_node_name.empty())) {
        issues->emplace_back(
                [p_name = pipe_name, n_name = selected_node_name](auto pcm) {
                    (*pcm)[p_name].delNode(n_name);
                });
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
        if (funcs.count(function_combo.text())) {
            ImGui::Separator();
            ImGui::Text("%s", getDescription(funcs[function_combo.text()]));
            ImGui::Separator();
        }
        if (ImGui::Button(label("OK"))) {
            issues->emplace_back([p_name = pipe_name,
                                  f_name = function_combo.text(),
                                  n_name = selected_node_name](auto pcm) {
                (*pcm)[p_name].allocateFunc(f_name, n_name);
            });
            ImGui::CloseCurrentPopup();
        }
    }

    edit_input_output_f |= GetIsKeyPressed('i', true);
    if (auto raii = BeginPopupModal(label("Edit input/output"),
                                    edit_input_output_f)) {
        ImGui::BeginGroup();
        int size = input_arg_name_its.size();
        if (ImGui::InputInt(label("input size"), &size)) {
            size = std::max(size, 0);
            input_arg_name_its.resize(size_t(size));
        }
        int i = 0;
        bool ok_f = false;
        for (auto& it : input_arg_name_its) {
            ok_f |= it.draw(label("input" + std::to_string(i++)));
        }
        if (ImGui::Button(label("OK##ine")) || ok_f) {
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
            size = std::max(size, 0);
            output_arg_name_its.resize(size_t(size));
        }
        i = 0;
        ok_f = false;
        for (auto& it : output_arg_name_its) {
            ok_f |= it.draw(label("output" + std::to_string(i++)));
        }
        if (ImGui::Button(label("OK##oute")) || ok_f) {
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

void EditWindow::drawEasyNodeGenarater(LabelWrapper label, Issues* issues) {
    if (auto raii = BeginPopupModal(label("easy node generater"),
                                    GetIsKeyDown('e', true), false)) {
        auto count = 0;
        std::string hovered_f_name;
        ImGui::BeginGroup();
        for (auto& f_name : f_names) {
            if (ImGui::Selectable(label(f_name), false,
                                  ImGuiSelectableFlags_DontClosePopups)) {
                issues->emplace_back([p_name = pipe_name,
                                      f_name = f_name](auto pcm) {
                    auto name = GetEasyName(f_name, (*pcm)[p_name].getNodes());
                    (*pcm)[p_name].newNode(name);
                    (*pcm)[p_name].allocateFunc(f_name, name);
                });
            }
            count++;
            if (ImGui::IsItemHovered()) {
                hovered_f_name = f_name;
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        for (int i = 0; i < count; i++) {
            ImGui::TextDisabled("   (click me!)");
        }
        ImGui::EndGroup();
        if (!GetIsKeyDown('e', true)) {
            ImGui::CloseCurrentPopup();
        }
        if (!hovered_f_name.empty()) {
            ImGui::Separator();
            ImGui::Text("%s", getDescription(funcs[hovered_f_name]));
        }
    }
}

void EditWindow::drawCanvasContextMenu(const string& hovered,
                                       LabelWrapper label, Issues* issues) {
    label.addSuffix("##CContext");
    bool modal_f = false;
    bool run_f = false;
    if (auto raii = BeginPopupContext(label("canvas"), hovered.empty(), 1)) {
        ImGui::MenuItem(label("new node"), "Ctrl-a", &modal_f);
        ImGui::TextDisabled("Press to Ctrl-e to open \"easy node generater\"");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::MenuItem(label("run"), "Ctrl-r", &run_f);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::MenuItem(label("small node mode"), "", &small_node_mode);
    }

    // run this pipeline.
    if (run_f || GetIsKeyPressed('r', true)) {
        issues->emplace_back([this](auto pcm) {
            WrapError([&] { (*pcm)[pipe_name].run(&report); }, &err_message);
        });
    }

    // new node maker popup.
    modal_f |= GetIsKeyPressed('a', true);
    if (auto raii = BeginPopupModal(label("new node popup"), modal_f)) {
        if (new_node_name_it.draw(label("new name"))) {
            issues->emplace_back([p_name = pipe_name,
                                  n_name = new_node_name_it.text()](auto pcm) {
                (*pcm)[p_name].newNode(n_name);
            });
            ImGui::CloseCurrentPopup();
        }
    }

    // Easy Node generater popup.
    drawEasyNodeGenarater(label, issues);
}

void EditWindow::drawEditPannel(const PipelineAPI& core_api, LabelWrapper label,
                                Issues* issues, VarEditors* var_editors) {
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

    if (GetIsKeyPressed('q')) {
        selected_node_name = "";
    }

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
    funcs = cm.getFunctionUtils(p_name);
    f_names.clear();
    f_names.reserve(funcs.size() + 1);
    for (auto& [f_name, f_util] : funcs) {
        if (f_name != p_name && !IsSpecialFuncName(f_name)) {
            f_names.emplace_back(f_name);
        }
    }
    function_combo.set(vector<string>(f_names));

    updateGuiNodeUtils(cm[p_name]);

    for (auto& layer : cm.getDependingTree().getDependenceLayer(p_name)) {
        for (auto& sub_p_name : layer) {
            children[sub_p_name];
        }
    }

    pipe_name = p_name;
}

void EditWindow::drawReportPannel(const PipelineAPI& core_api,
                                  LabelWrapper label, Issues* issues) {
    DrawCanvas(ImVec2(0, 0), 100);
    const ImVec2 canvas_offset = ImGui::GetCursorScreenPos();
    for (auto& [n_name, node] : core_api.getNodes()) {
        GuiNode& gui_node = node_gui_utils[n_name];
        // Draw node box
        ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos);
        drawNodeBox(canvas_offset + gui_node.pos, gui_node.size,
                    selected_node_name == n_name, gui_node.id, label);
        Report& repo = report.child_reports[n_name];
        float wait = std::min(1.f, repo.getSec() / report.getSec());
        wait = std::min(1.f, repo.getSec() / report.getSec());
        if (report.getSec() == 0.f) {
            wait = 0.f;
            continue;
        }
        if (wait != 0.f) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            auto start = canvas_offset + gui_node.pos;
            auto end = canvas_offset + gui_node.pos +
                       gui_node.size * ImVec2(wait, 1);
            draw_list->AddRectFilled(
                    start, end,
                    ImGui::GetColorU32(VolumeColor(1.f - wait) *
                                       ImVec4(1, 1, 1, 0.4)),
                    4.f);
        }

        if (!small_node_mode || selected_node_name == n_name) {
            ImGui::SetCursorScreenPos(canvas_offset + gui_node.pos +
                                      NODE_WINDOW_PADDING);
            ImGui::BeginGroup();
            ImGui::Text("%s", n_name.c_str());
            ImGui::Dummy(ImVec2(0.f, NODE_WINDOW_PADDING.y));
            ImGui::TextColored(ImVec4(1, 1, 1, 1), "%.3f mili sec (%.3f %%)",
                               repo.getSec() * 1000.0, wait * 100.0);
            ImGui::EndGroup();
        }
    }

    // run this pipeline.
    if (GetIsKeyPressed('r', true)) {
        issues->emplace_back([this](auto pcm) {
            WrapError([&] { (*pcm)[pipe_name].run(&report); }, &err_message);
        });
    }
}

void EditWindow::drawContent(const CoreManager& cm, LabelWrapper label,
                             Issues* issues, VarEditors* var_editors) {
    if (ImGui::BeginTabBar(label("TabBar"))) {
        if (ImGui::BeginTabItem(label("Edit"))) {
            drawEditPannel(cm[pipe_name], label, issues, var_editors);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(label("Report"))) {
            drawReportPannel(cm[pipe_name], label, issues);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (auto p_raii = BeginPopupModal(label("Error"), !err_message.empty())) {
        ImGui::Text("%s", err_message.c_str());
        if (ImGui::Button(label("OK"))) {
            ImGui::CloseCurrentPopup();
            err_message = "";
        }
    }
}

void EditWindow::drawChild(const CoreManager& cm, const string& child_p_name,
                           EditWindow& child, LabelWrapper label,
                           Issues* issues, VarEditors* var_editors) {
    child.updateMembers(cm, child_p_name);
    child.small_node_mode = small_node_mode;
    child.drawContent(cm, label, issues, var_editors);
    small_node_mode = child.small_node_mode;
}

bool EditWindow::draw(const string& p_name, const string& win_title,
                      const CoreManager& cm, LabelWrapper label, Issues* issues,
                      VarEditors* var_editors) {
    label.addSuffix("##" + p_name);
    updateMembers(cm, p_name);

    bool opened = true;
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
    if (auto raii = WindowRAII(label(win_title + " : Pipe Edit - " + p_name),
                               &opened)) {
        if (children.empty()) {
            drawContent(cm, label, issues, var_editors);
        } else {
            if (ImGui::BeginTabBar(label("pipe_tab"))) {
                if (ImGui::BeginTabItem(label(p_name))) {
                    drawContent(cm, label, issues, var_editors);
                    ImGui::EndTabItem();
                }
                for (auto& [sub_p_name, child] : children) {
                    if (ImGui::BeginTabItem(label(sub_p_name))) {
                        drawChild(cm, sub_p_name, child, label, issues,
                                  var_editors);
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
    }

    return opened;
}

} // namespace fase
