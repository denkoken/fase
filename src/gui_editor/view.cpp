
#include "view.h"

#include <cmath>
#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core_util.h"

namespace fase {

namespace {

template <typename T>
bool erase(std::vector<T>& vec, const T& item) {
    for (auto iter = std::begin(vec); iter != std::end(vec); iter++) {
        if (item == *iter) {
            vec.erase(iter);
            return true;
        }
    }
    return false;
}

template <typename T>
inline size_t getIndex(const std::vector<T>& vec, const T& item) {
    return size_t(std::find(std::begin(vec), std::end(vec), item) -
                  std::begin(vec));
}

template <typename Key, typename T>
std::vector<Key> getKeys(const std::map<Key, T>& map) {
    std::vector<Key> keys;
    for (const auto& pair : map) {
        keys.push_back(std::get<0>(pair));
    }
    return keys;
}

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

const Function& getFunction(const FaseCore& core,
                            const std::string& node_name) {
    return core.getFunctions().at(core.getNodes().at(node_name).func_repr);
}

std::string WrapNodeName(const std::string& node_name) {
    if (node_name == InputNodeStr()) {
        return "Input";
    } else if (node_name == OutputNodeStr()) {
        return "Output";
    }
    return node_name;
}

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

void DrawColTextBox(const ImU32& col, const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    ImGui::Button(text);
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
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

float GetVolume(const int& idx) {
    if (idx == 0)
        return 0.f;
    else if (idx == 1)
        return 1.f;
    else {
        int i = std::pow(2, int(std::log2(idx - 1)));
        return ((idx - i) * 2 - 1) / 2.f / float(i);
    }
}

constexpr float NODE_COL_SCALE = 155.f;
constexpr float NODE_COL_OFFSET = 100.f;

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

class CanvasController {
public:
    CanvasController(const char* label) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg,
                              IM_COL32(60, 60, 70, 200));
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
    }
};

struct CanvasState {
    ImVec2 scroll_pos;
    std::map<std::string, GuiNode> gui_nodes;

    std::string hovered_slot_name = "";
    size_t hovered_slot_idx = 0;
    bool is_hovered_slot_input = false;
    bool is_link_creating = false;
};

class GUINodePositionOptimizer {
public:
    GUINodePositionOptimizer(const FaseCore& core,
                             std::map<std::string, GuiNode>& gui_nodes)
        : core(core), gui_nodes(gui_nodes) {}

    void operator()(bool opt) {
        if (opt) {
            setDestinations();
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
    static constexpr float START_X = 30;
    static constexpr float START_Y = 10;
    static constexpr float INTERVAL_X = 50;
    static constexpr float INTERVAL_Y = 10;

    const FaseCore& core;

    std::map<std::string, ImVec2> destinations;
    std::map<std::string, GuiNode>& gui_nodes;

    void setDestinations() {
        auto names = GetCallOrder(core.getNodes());

        float baseline_y = 0;
        float x = START_X, y = START_Y, maxy = 0;
        ImVec2 component_size = ImGui::GetWindowSize();
        for (auto& name_set : names) {
            float maxx = 0;
            for (auto& name : name_set) {
                maxx = std::max(maxx, gui_nodes.at(name).size.x);
            }
            if (x + maxx > component_size.x) {
                x = START_X;
                baseline_y = maxy + INTERVAL_Y * 0.7f + baseline_y;
            }
            for (auto& name : name_set) {
                destinations[name].y = y + baseline_y;
                destinations[name].x = x;
                y += gui_nodes.at(name).size.y + INTERVAL_Y;
            }
            maxy = std::max(maxy, y);
            y = START_Y;
            x += maxx + INTERVAL_X;
        }
    }
};

}  // namespace

class ReportWindow : public Content {
public:
    template <class... Args>
    ReportWindow(Args&&... args) : Content(args...) {}
    ~ReportWindow() {}

private:
    enum class ViewList {
        Nodes,
        Steps,
    };
    enum class SortRule : int {
        Name,
        Time,
    };
    ViewList view = ViewList::Nodes;
    SortRule sort_rule = SortRule::Name;
    std::map<std::string, ResultReport> report_box;

    std::function<bool(const std::pair<std::string, ResultReport>&)>
    getFilter() {
        if (view == ViewList::Nodes) {
            return [](const auto& v) {
                return std::get<0>(v).find(ReportHeaderStr()) ==
                       std::string::npos;
            };
        } else if (view == ViewList::Steps) {
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
        if (view == ViewList::Nodes) {
            float total = 0.f;
            for (const auto& report : reports) {
                total += getSec(std::get<1>(report));
            }
            return total;
        } else if (view == ViewList::Steps) {
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

    void main() {
        std::map<std::string, ResultReport>* report_pp = nullptr;
        if (getResponse(REPORT_RESPONSE_ID, &report_pp)) {
            report_box = *report_pp;
            ImGui::SetNextWindowFocus();
        }
        if (report_box.empty()) {
            return;
        }

        ImGui::Begin("Report", NULL, ImGuiWindowFlags_MenuBar);

        ImGui::BeginMenuBar();

        if (ImGui::MenuItem(label("Nodes"), NULL, view == ViewList::Nodes)) {
            view = ViewList::Nodes;
        }
        if (ImGui::MenuItem(label("Steps"), NULL, view == ViewList::Steps)) {
            view = ViewList::Steps;
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
        auto keys = getKeys(reports);
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
};

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

        // List component
        ImGui::PushID(label(WrapNodeName(node_name)));

        std::stringstream view_ss;
        if (node.func_repr == OutputFuncStr()) {
            view_ss << "Output";
        } else if (node.func_repr == InputFuncStr()) {
            view_ss << "Input";
        } else {
            view_ss << idx - 2 << ") " << node_name;
            view_ss << " [" + node.func_repr + "]";
        }
        if (ImGui::Selectable(label(view_ss.str()),
                              exists(state.selected_nodes, node_name))) {
            if (IsKeyPressed(ImGuiKey_Space)) {
                state.selected_nodes.push_back(node_name);
            } else {
                state.selected_nodes = {node_name};
            }
        }
        if (ImGui::IsItemHovered()) {
            state.hovered_node_name = node_name;
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

class LinksView : public Content {
public:
    template <class... Args>
    LinksView(CanvasState& canvas_state, Args&&... args)
        : Content(args...),
          c_state(canvas_state),
          gui_nodes(canvas_state.gui_nodes) {}
    ~LinksView() {}

private:
    constexpr static ImU32 LINK_COLOR = IM_COL32(200, 200, 100, 255);
    constexpr static float SLOT_HOVER_RADIUS = 8.f;
    // const float ARROW_BEZIER_SIZE = 80.f;
    constexpr static float ARROW_HEAD_SIZE = 15.f;
    const float ARROW_HEAD_X_OFFSET = -ARROW_HEAD_SIZE * std::sqrt(3.f) * 0.5f;

    CanvasState& c_state;
    std::map<std::string, GuiNode>& gui_nodes;

    std::string hovered_slot_name_prev = "";
    size_t hovered_slot_idx_prev = 0;
    bool is_hovered_slot_input_prev = false;

    void drawLink(const ImVec2& s_pos, const ImVec2& d_pos,
                  const size_t& order) {
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
                                  GenNodeColor(order), 3.0f);
        // Arrow's triangle
        const ImVec2 t_pos_1 =
                d_pos + ImVec2(ARROW_HEAD_X_OFFSET, ARROW_HEAD_SIZE * 0.5f);
        const ImVec2 t_pos_2 =
                d_pos + ImVec2(ARROW_HEAD_X_OFFSET, -ARROW_HEAD_SIZE * 0.5f);
        draw_list->AddTriangleFilled(d_pos, t_pos_1, t_pos_2,
                                     GenNodeColor(order));
    }

    void main() {
        // TODO
        const std::map<std::string, Node>& nodes = core.getNodes();

        // Draw links
        for (const std::string& node_name : state.node_order) {
            const Node& node = nodes.at(node_name);
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
                const ImVec2 s_pos =
                        gui_nodes.at(src_name).getOutputSlot(src_idx);
                // Destination
                const ImVec2 d_pos =
                        gui_nodes.at(node_name).getInputSlot(dst_idx);

                const size_t src_node_idx = getIndex(
                        state.node_order, node.links[dst_idx].node_name);

                drawLink(s_pos, d_pos, src_node_idx);
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
            GuiNode& gui_node = gui_nodes.at(node_name);
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
                    c_state.hovered_slot_name = node_name;
                    c_state.hovered_slot_idx = arg_idx;
                    c_state.is_hovered_slot_input = inp_hov;
                }
            }
        }

        // Link creation
        if (c_state.is_link_creating) {
            if (ImGui::IsMouseDragging(0, 0.f)) {
                // Editing
                const GuiNode& gui_node = gui_nodes.at(hovered_slot_name_prev);
                const size_t& arg_idx = hovered_slot_idx_prev;
                if (is_hovered_slot_input_prev) {
                    drawLink(
                            mouse_pos, gui_node.getInputSlot(arg_idx),
                            getIndex(state.node_order, hovered_slot_name_prev));
                } else {
                    drawLink(
                            gui_node.getOutputSlot(arg_idx), mouse_pos,
                            getIndex(state.node_order, hovered_slot_name_prev));
                }
            } else {
                if (c_state.hovered_slot_name.empty() ||
                    c_state.is_hovered_slot_input ==
                            is_hovered_slot_input_prev) {
                    // Canceled
                } else {
                    // Create link
                    bool success;
                    AddLinkInfo info;
                    if (is_hovered_slot_input_prev) {
                        info = {c_state.hovered_slot_name,
                                c_state.hovered_slot_idx,
                                hovered_slot_name_prev, hovered_slot_idx_prev};
                    } else {
                        info = {hovered_slot_name_prev, hovered_slot_idx_prev,
                                c_state.hovered_slot_name,
                                c_state.hovered_slot_idx};
                    }
                    throwIssue(IssuePattern::AddLink, true, info, &success);
                }
                c_state.is_link_creating = false;
            }
        } else {
            if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0, 1.f) &&
                !c_state.hovered_slot_name.empty()) {
                // Start creating
                c_state.is_link_creating = true;
                const Link& link = nodes.at(c_state.hovered_slot_name)
                                           .links[c_state.hovered_slot_idx];
                if (link.node_name.empty() || !c_state.is_hovered_slot_input) {
                    // New link
                    hovered_slot_name_prev = c_state.hovered_slot_name;
                    hovered_slot_idx_prev = c_state.hovered_slot_idx;
                    is_hovered_slot_input_prev = c_state.is_hovered_slot_input;
                } else {
                    // Edit existing link
                    hovered_slot_name_prev = link.node_name;
                    hovered_slot_idx_prev = link.arg_idx;
                    is_hovered_slot_input_prev = false;
                    bool success;
                    throwIssue(IssuePattern::DelLink, true,
                               DelLinkInfo{c_state.hovered_slot_name,
                                           c_state.hovered_slot_idx},
                               &success);
                }
            }
        }
    }
};

class NodeBoxesView : public Content {
public:
    template <class... Args>
    NodeBoxesView(const std::map<const std::type_info*, VarEditor>& var_editors,
                  CanvasState& c_state, Args&&... args)
        : Content(args...), var_editors(var_editors), c_state(c_state) {}
    ~NodeBoxesView() {}

private:
    const float SLOT_RADIUS = 4.f;
    const float SLOT_SPACING = 3.f;
    const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 6.f);
    const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
    const ImU32 BG_NML_COLOR = IM_COL32(60, 60, 60, 255);
    const ImU32 BG_ACT_COLOR = IM_COL32(75, 75, 75, 255);
    const ImU32 SLOT_NML_COLOR = IM_COL32(240, 240, 150, 150);
    const ImU32 SLOT_ACT_COLOR = IM_COL32(255, 255, 100, 255);

    const std::map<const std::type_info*, VarEditor>& var_editors;

    CanvasState& c_state;

    std::string getTitleText(const std::string& node_name, const Node& node,
                             const size_t& order_idx) {
        if (node.func_repr == InputFuncStr()) {
            return "Input";
        } else if (node.func_repr == OutputFuncStr()) {
            return "Output";
        } else {
            std::stringstream sstream;
            sstream << "(" << order_idx - 2 << ") ";
            if (!preference.is_simple_node_box) {
                sstream << "[" << node.func_repr << "] ";
            }
            sstream << node_name;
            return sstream.str();
        }
    }

    void drawNodeContent(const std::string& node_name, const Node& node,
                         GuiNode& gui_node, const size_t& order_idx,
                         bool simple) {
        ImGui::BeginGroup();  // Lock horizontal position
        bool s_flag = IsSpecialFuncName(node.func_repr);

        ImGui::Text("%s", getTitleText(node_name, node, order_idx).c_str());

        ImGui::Dummy(ImVec2(0.f, NODE_WINDOW_PADDING.y));

        const size_t n_args = node.links.size();
        assert(gui_node.arg_poses.size() == n_args);

        // Draw arguments
        const std::map<std::string, Function>& functions = core.getFunctions();
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
            DrawInOutTag(function.is_input_args[arg_idx], simple);

            // draw default argument editing
            ImGui::SameLine();
            if (simple) {
                if (int(arg_name.size()) > preference.max_arg_name_chars) {
                    std::string view(std::begin(arg_name),
                                     std::begin(arg_name) +
                                             preference.max_arg_name_chars - 2);
                    view += "..";
                    ImGui::Text("%s", view.c_str());
                } else {
                    ImGui::Text("%s", arg_name.c_str());
                }
            } else if (!node.links[arg_idx].node_name.empty()) {
                // Link exists
                ImGui::Text("%s", arg_name.c_str());
            } else if (var_editors.count(arg_type)) {
                // Call registered GUI for editing
                auto& func = var_editors.at(arg_type);
                const Variable& var = node.arg_values[arg_idx];
                std::string expr;
                const std::string view_label = arg_name + "##" + node_name;
                Variable v = func(label(view_label), var);
                throwIssue(IssuePattern::SetArgValue, bool(v),
                           SetArgValueInfo{node_name, arg_idx, "", v});
            } else {
                // No GUI for editing
                if (s_flag) {
                    ImGui::Text("%s", arg_name.c_str());
                } else {
                    ImGui::Text("%s [default:%s]", arg_name.c_str(),
                                arg_repr.c_str());
                }
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(SLOT_SPACING, 0));
        }
        if (!s_flag && !simple) {
            int priority = node.priority;
            ImGui::PushItemWidth(100);
            ImGui::SliderInt(label("priority"), &priority,
                             preference.priority_min, preference.priority_max);
            ImGui::PopItemWidth();
            priority = std::min(priority, preference.priority_max);
            priority = std::max(priority, preference.priority_min);
            if (priority != node.priority) {
                throwIssue(IssuePattern::SetPriority, true,
                           SetPriorityInfo{node_name, priority});
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
                                 GenNodeColor(order_idx), 4.f);
    }

    void drawLinkSlots(const GuiNode& gui_node, const size_t& order_idx) {
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
            const ImU32 SLOT_NML_COLOR = GenNodeColor(order_idx);
            const ImU32 inp_col = inp_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
            const ImU32 out_col = out_hov ? SLOT_ACT_COLOR : SLOT_NML_COLOR;
            draw_list->AddCircleFilled(inp_slot, SLOT_RADIUS, inp_col);
            draw_list->AddCircleFilled(out_slot, SLOT_RADIUS, out_col);
        }
    }

    void main() {
        const ImVec2 canvas_offset =
                ImGui::GetCursorScreenPos() + c_state.scroll_pos;
        auto& gui_nodes = c_state.gui_nodes;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->ChannelsSplit(2);
        const std::map<std::string, Node>& nodes = core.getNodes();
        for (size_t order_idx = 0; order_idx < state.node_order.size();
             order_idx++) {
            const std::string& node_name = state.node_order[order_idx];
            const Node& node = nodes.at(node_name);
            if (!gui_nodes.count(node_name)) {
                continue;  // Wait for creating GUI node
            }
            GuiNode& gui_node = gui_nodes[node_name];

            ImGui::PushID(label(node_name));
            const ImVec2 node_rect_min = canvas_offset + gui_node.pos;
            const bool any_active_old = ImGui::IsAnyItemActive();

            // Draw node contents first
            draw_list->ChannelsSetCurrent(1);  // Foreground
            ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
            drawNodeContent(node_name, node, gui_node, order_idx,
                            preference.is_simple_node_box);

            // Fit to content size
            gui_node.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING * 2;

            // Draw node box
            draw_list->ChannelsSetCurrent(0);  // Background
            ImGui::SetCursorScreenPos(node_rect_min);
            drawNodeBox(node_rect_min, gui_node.size,
                        exists(state.selected_nodes, node_name), order_idx);

            // Draw link slot
            drawLinkSlots(gui_node, order_idx);

            // Selection
            if (!any_active_old && ImGui::IsAnyItemActive()) {
                if (IsKeyPressed(ImGuiKey_Space)) {
                    if (!exists(state.selected_nodes, node_name)) {
                        state.selected_nodes.push_back(node_name);
                    }
                } else {
                    state.selected_nodes = {node_name};
                }
            }
            if (ImGui::IsItemHovered()) {
                state.hovered_node_name = node_name;
            }
            // Scroll
            if (!c_state.is_link_creating && ImGui::IsItemActive() &&
                ImGui::IsMouseDragging(0, 0.f)) {
                gui_node.pos = gui_node.pos + ImGui::GetIO().MouseDelta;
            }

            ImGui::PopID();
        }
        draw_list->ChannelsMerge();
    }
};

class ContextMenu : public Content {
public:
    template <class... Args>
    ContextMenu(CanvasState& c_state, Args&&... args)
        : Content(args...), c_state(c_state) {}
    ~ContextMenu() {}

private:
    // Reference to the parent's
    CanvasState& c_state;

    // Private status
    std::string selected_slot_name;
    size_t selected_slot_idx = 0;
    bool is_selected_slot_input = false;

    char new_node_name[64] = "";

    void renamePopUp(const char* popup_name) {
        bool opened = true;
        if (ImGui::BeginPopupModal(label(popup_name), &opened,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            ImGui::InputText(label("New node name (ID)"), new_node_name,
                             sizeof(new_node_name));

            bool success;
            if (issueButton(
                        IssuePattern::RenameNode,
                        RenameNodeInfo{state.selected_nodes[0], new_node_name},
                        &success, "OK")) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void main() {
        // Right click
        if (ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows) &&
            ImGui::IsMouseClicked(1)) {
            if (!c_state.hovered_slot_name.empty()) {
                // Open pop up window for a slot
                state.selected_nodes = {c_state.hovered_slot_name};
                selected_slot_name = c_state.hovered_slot_name;
                selected_slot_idx = c_state.hovered_slot_idx;
                is_selected_slot_input = c_state.is_hovered_slot_input;
                ImGui::OpenPopup(label("Popup: Slot context menu"));
            } else if (!state.hovered_node_name.empty()) {
                // Open pop up window for a node
                state.selected_nodes = {state.hovered_node_name};
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
                    throwIssue(
                            IssuePattern::DelLink, true,
                            DelLinkInfo{selected_slot_name, selected_slot_idx});
                } else {
                    // Remove output links
                    std::vector<std::pair<std::string, size_t>> slots;
                    FindConnectedOutSlots(core.getNodes(),
                                          state.selected_nodes[0],
                                          selected_slot_idx, slots);
                    for (auto& it : slots) {
                        throwIssue(IssuePattern::DelLink, true,
                                   DelLinkInfo{it.first, it.second});
                    }
                }
            }
            ImGui::EndPopup();
        }

        // Node menu
        if (ImGui::BeginPopup(label("Popup: Node context menu"))) {
            ImGui::Text("Node \"%s\"", state.selected_nodes[0].c_str());
            ImGui::Separator();
            throwIssue(IssuePattern::DelNode, ImGui::MenuItem(label("Delete")),
                       state.selected_nodes[0]);
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
        renamePopUp("Popup: Rename node");

        // Node menu
        if (ImGui::BeginPopup(label("Popup: Common context menu"))) {
            if (ImGui::MenuItem(label("Add node"))) {
                // Call for another class `NodeAddingGUI`
                // request_add_node = true;
            }
            ImGui::EndPopup();
        }
    }
};

class NodeCanvasView : public Content {
public:
    template <class... Args>
    NodeCanvasView(
            const std::map<const std::type_info*, VarEditor>& var_editors,
            Args&&... args)
        : Content(args...),
          links_view(c_state, args...),
          node_boxes_view(var_editors, c_state, args...),
          context_menu(c_state, args...),
          position_optimizer(core, c_state.gui_nodes) {}
    ~NodeCanvasView() {}

private:
    // child contents
    LinksView links_view;
    NodeBoxesView node_boxes_view;
    ContextMenu context_menu;

    CanvasState c_state;
    std::string prev_nodes_str;  // for check is updating core nodes.
    GUINodePositionOptimizer position_optimizer;

    void updateCanvasState() {
        state.hovered_node_name.clear();
        c_state.hovered_slot_name.clear();
    }

    void updateGuiNodes() {
        auto& gui_nodes = c_state.gui_nodes;

        const std::map<std::string, Node>& nodes = core.getNodes();
        for (auto it = nodes.begin(); it != nodes.end(); it++) {
            const std::string& node_name = it->first;
            const size_t n_args = it->second.links.size();
            // Check GUI node existence
            if (!gui_nodes.count(node_name)) {
                gui_nodes[node_name];
            }
            if (gui_nodes[node_name].arg_size() != n_args) {
                // Create new node and allocate for link slots
                gui_nodes[node_name].alloc(n_args);
            }
        }
        // Detect changes by the numbers of nodes and links
        std::string nodes_str = CoreToString(core, {});
        if (nodes_str != prev_nodes_str) {
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
        }
#if 0
        // Save argument positions
        const static ImVec2 NODE_WINDOW_PADDING = NODE_WINDOW_PADDING
        for (auto& pair : gui_nodes) {
            GuiNode& gui_node = std::get<1>(pair);
            for (size_t arg_idx = 0; arg_idx < gui_node.arg_poses.size(); arg_idx++) {
                const ImVec2 pos = ImGui::GetCursorScreenPos();
                gui_node.arg_poses[arg_idx] =
                        ImVec2(pos.x - NODE_WINDOW_PADDING.x,
                               pos.y + ImGui::GetTextLineHeight() * 0.5f);
            }
        }
#endif

        // optimize gui node positions
        position_optimizer(preference.auto_layout);
    }

    void main();
};

void NodeCanvasView::main() {
    updateGuiNodes();
    updateCanvasState();

    ImGui::Text("Hold middle mouse button to scroll (%f, %f)",
                c_state.scroll_pos.x, c_state.scroll_pos.y);
    {
        CanvasController cc(label("scrolling_region"));

        // Draw grid canvas
        DrawCanvas(c_state.scroll_pos, 64.f);
        // Draw links
        drawChild(links_view);
        // Draw nodes
        drawChild(node_boxes_view);

        // Canvas scroll
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseDragging(2, 0.f)) {
            c_state.scroll_pos = c_state.scroll_pos + ImGui::GetIO().MouseDelta;
        }
        // Clear selected node
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&
            ImGui::IsMouseClicked(0)) {
            state.selected_nodes.clear();
        }
    }

    drawChild(context_menu);
}

// For Left panel
class NodeArgEditView : public Content {
public:
    template <class... Args>
    NodeArgEditView(
            const std::map<const std::type_info*, VarEditor>& var_editors,
            Args&&... args)
        : Content(args...), var_editors(var_editors) {}
    ~NodeArgEditView() {}

private:
    // type and name
    using argIdentiry = std::tuple<const std::type_info*, std::string, bool>;

    const std::map<const std::type_info*, VarEditor>& var_editors;

    std::vector<argIdentiry> getEdittingArgs() {
        std::vector<argIdentiry> args;
        {
            const Node& node = core.getNodes().at(state.selected_nodes[0]);
            const Function& function = core.getFunctions().at(node.func_repr);
            for (size_t i = 0; i < node.arg_reprs.size(); i++) {
                args.emplace_back(argIdentiry{function.arg_types[i],
                                              function.arg_names[i],
                                              function.is_input_args[i]});
            }
        }

        for (size_t i = 1; i < state.selected_nodes.size(); i++) {
            std::vector<argIdentiry> args_buf;
            const Node& node = core.getNodes().at(state.selected_nodes[i]);
            const Function& function = core.getFunctions().at(node.func_repr);
            for (size_t j = 0; j < node.arg_reprs.size(); j++) {
                argIdentiry arg = {function.arg_types[j], function.arg_names[j],
                                   function.is_input_args[j]};
                if (exists(args, arg)) {
                    args_buf.emplace_back(std::move(arg));
                }
            }
            args = args_buf;
        }
        return args;
    }

    void drawVarEditor(const argIdentiry& arg) {
        const std::string arg_name = std::get<1>(arg);
        const std::type_info* arg_type = std::get<0>(arg);
        // Call registered GUI for editing
        auto& func = var_editors.at(arg_type);
        const Node& node = core.getNodes().at(state.selected_nodes[0]);
        const Function& f = core.getFunctions().at(node.func_repr);
        size_t arg_idx = getIndex(f.arg_names, arg_name);
        const Variable& var = node.arg_values[arg_idx];
        std::string expr;
        const std::string view_label = arg_name;
        Variable v = func(label(view_label), var);
        for (const std::string& node_name : state.selected_nodes) {
            const Function& f_ = getFunction(core, node_name);

            size_t idx = getIndex(f_.arg_names, arg_name);

            throwIssue(IssuePattern::SetArgValue, bool(v),
                       SetArgValueInfo{node_name, idx, "", v});
        }
    }

    void main() {
        ImGui::Separator();
        ImGui::Text("Argument Editor");
        ImGui::Separator();

        if (state.selected_nodes.empty()) {
            return;
        }

        for (const std::string& node_name : state.selected_nodes) {
            DrawColTextBox(GenNodeColor(getIndex(state.node_order, node_name)),
                           label(WrapNodeName(node_name).c_str()));
            ImGui::SameLine();
        }
        ImGui::Text("");
        ImGui::Separator();

        std::vector<argIdentiry> args = getEdittingArgs();

        for (auto& arg : args) {
            DrawInOutTag(std::get<2>(arg), false);
            // draw default argument editing
            if (var_editors.count(std::get<0>(arg))) {
                ImGui::SameLine();
                drawVarEditor(arg);
            }
        }

        // Edit Priority
        const Node& node = core.getNodes().at(state.selected_nodes[0]);
        int priority = node.priority;

        ImGui::PushItemWidth(-100);
        ImGui::SliderInt(label("priority"), &priority, preference.priority_min,
                         preference.priority_max);
        ImGui::PopItemWidth();
        priority = std::min(priority, preference.priority_max);
        priority = std::max(priority, preference.priority_min);
        if (priority != node.priority) {
            for (const auto& node_name : state.selected_nodes) {
                throwIssue(IssuePattern::SetPriority, true,
                           SetPriorityInfo{node_name, priority});
            }
        }
    }
};

View::View(const FaseCore& core, const TypeUtils& utils,
           const std::map<const std::type_info*, VarEditor>& var_editors)
    : core(core), utils(utils), var_editors(var_editors) {
    auto add_issue_function = [this](auto&& a) { issues.emplace_back(a); };
    node_list = std::make_unique<NodeListView>(core, label, state, utils,
                                               add_issue_function);
    canvas = std::make_unique<NodeCanvasView>(var_editors, core, label, state,
                                              utils, add_issue_function);
    args_editor = std::make_unique<NodeArgEditView>(
            var_editors, core, label, state, utils, add_issue_function);
    report_window = std::make_unique<ReportWindow>(core, label, state, utils,
                                                   add_issue_function);
    setupMenus(add_issue_function);
}

View::~View() = default;

void View::updateState() {
    // Update Node Order.
    const std::map<std::string, Node>& nodes = core.getNodes();
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        const std::string& node_name = it->first;
        // Check GUI node existence
        if (!exists(state.node_order, node_name)) {
            state.node_order.emplace_back(node_name);
        }
    }
    while (true) {
        bool f = true;
        for (const std::string& name : state.node_order) {
            if (!nodes.count(name)) {
                erase(state.node_order, name);
                f = false;
            }
        }
        if (f) {
            break;
        }
    }

    // reflesh state.selected_nodes
    while (true) {
        bool f = true;
        for (const std::string& name : state.selected_nodes) {
            if (!nodes.count(name)) {
                erase(state.selected_nodes, name);
                f = false;
            }
        }
        if (f) {
            break;
        }
    }
}

std::vector<Issue> View::draw(const std::string& win_title,
                              const std::string& label_suffix,
                              const std::map<std::string, Variable>& resp) {
    issues.clear();
    updateState();

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
            ImGui::Dummy(ImVec2(5, 0));  // Spacing
        }
        ImGui::EndMenuBar();
    }

    // Left Panel
    if (state.preference.enable_node_list_panel) {
        ImGui::BeginChild(label("left panel"),
                          ImVec2(state.preference.node_list_panel_size, 0));
        // Draw a list of nodes on the left side
        node_list->draw(resp);
        ImGui::EndChild();
        ImGui::SameLine();
    }

    // Center Panel
    if (state.preference.enable_edit_panel) {
        ImGui::BeginChild(label("center panel"),
                          ImVec2(state.preference.edit_panel_size, 0));
        // Draw a list of nodes on the left side
        args_editor->draw(resp);
        ImGui::EndChild();

        ImGui::SameLine();
    }

    // Right Panel
    ImGui::BeginChild(label("right canvas"));
    canvas->draw(resp);
    ImGui::EndChild();

    ImGui::End();  // End window

    report_window->draw(resp);

    return issues;
}

}  // namespace fase
