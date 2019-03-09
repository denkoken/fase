
#include "links_view.h"

#include "../manager.h"

namespace fase {

using std::map, std::string, std::vector;
using size_t = std::size_t;

namespace {

std::tuple<string, size_t> GetSrcName(const string& dst_n_name,
                                      const size_t idx,
                                      const vector<Link>& links) {
    for (auto& link : links) {
        if (link.dst_node == dst_n_name && link.dst_arg == idx) {
            return {link.src_node, link.src_arg};
        }
    }
    return {};
}

}  // namespace

void LinksView::drawLink(const ImVec2& s_pos, const ImVec2& d_pos,
                         const size_t& d_id) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 diff = d_pos - s_pos;
    float x_d = std::abs(diff.x) * .7f;
    if (diff.x < 0.f) {
        x_d = std::pow(-diff.x, .7f) * 3.f;
    }
    const ImVec2 s_pos_2 = s_pos + ImVec2(x_d, 0);
    const ImVec2 d_pos_2 =
            d_pos - ImVec2(x_d, 0) - ImVec2(ARROW_HEAD_SIZE * 0.8f, 0);
    draw_list->AddBezierCurve(s_pos, s_pos_2, d_pos_2,
                              d_pos - ImVec2(ARROW_HEAD_SIZE * 0.8f, 0),
                              GenNodeColor(d_id), 3.0f);
    // Arrow's triangle
    const ImVec2 t_pos_1 =
            d_pos + ImVec2(ARROW_HEAD_X_OFFSET, ARROW_HEAD_SIZE * 0.5f);
    const ImVec2 t_pos_2 =
            d_pos + ImVec2(ARROW_HEAD_X_OFFSET, -ARROW_HEAD_SIZE * 0.5f);
    draw_list->AddTriangleFilled(d_pos, t_pos_1, t_pos_2, GenNodeColor(d_id));
}

void LinksView::draw(const map<string, Node>& nodes, const vector<Link>& links,
                     const string& p_name, map<string, GuiNode>& node_gui_utils,
                     Issues* issues) {
    // Draw links
    for (auto& [gn_name, gui_node] : node_gui_utils) {
        const size_t n_args = nodes.at(gn_name).args.size();
        for (size_t dst_idx = 0; dst_idx < n_args; dst_idx++) {
            auto [src_n_name, src_idx] = GetSrcName(gn_name, dst_idx, links);
            if (src_n_name.empty() || !node_gui_utils.count(src_n_name)) {
                continue;  // No link or Wait for creating GUI node
            }
            // Source
            const ImVec2 s_pos =
                    node_gui_utils.at(src_n_name).getOutputSlot(src_idx);
            // Destination
            const ImVec2 d_pos =
                    node_gui_utils.at(gn_name).getInputSlot(dst_idx);

            const size_t d_id = node_gui_utils.at(src_n_name).id;

            drawLink(s_pos, d_pos, d_id);
        }
    }

    hovered_slot_name = "";

    // Search hovered slot
    const ImVec2 mouse_pos = ImGui::GetMousePos();
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        const std::string& node_name = it->first;
        const Node& node = it->second;
        if (!node_gui_utils.count(node_name)) {
            continue;  // Wait for creating GUI node
        }
        GuiNode& gui_node = node_gui_utils.at(node_name);
        const size_t n_args = node.args.size();
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            // Get slot position
            const ImVec2 inp_slot = gui_node.getInputSlot(arg_idx) +
                                    ImVec2(1, 0) * SLOT_HOVER_RADIUS;
            const ImVec2 out_slot = gui_node.getOutputSlot(arg_idx) -
                                    ImVec2(1, 0) * SLOT_HOVER_RADIUS;
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
            const GuiNode& gui_node = node_gui_utils.at(hovered_slot_name_prev);
            const size_t& arg_idx = hovered_slot_idx_prev;
            if (is_hovered_slot_input_prev) {
                drawLink(mouse_pos, gui_node.getInputSlot(arg_idx),
                         gui_node.id);
            } else {
                drawLink(gui_node.getOutputSlot(arg_idx), mouse_pos,
                         gui_node.id);
            }
        } else {
            if (hovered_slot_name.empty() ||
                is_hovered_slot_input == is_hovered_slot_input_prev) {
                // Canceled
            } else {
                // Create link
                struct Linker {
                    string p_name, src, dst;
                    size_t s_idx, d_idx;
                    void operator()(CoreManager* pcm) {
                        (*pcm)[p_name].smartLink(src, s_idx, dst, d_idx);
                    }
                };
                if (is_hovered_slot_input_prev) {
                    issues->emplace_back(Linker{
                            p_name, hovered_slot_name, hovered_slot_name_prev,
                            hovered_slot_idx, hovered_slot_idx_prev});
                } else {
                    issues->emplace_back(Linker{
                            p_name, hovered_slot_name_prev, hovered_slot_name,
                            hovered_slot_idx_prev, hovered_slot_idx});
                }
            }
            hovered_slot_name_prev = "";
            hovered_slot_idx_prev = 0;
            is_link_creating = false;
        }
    } else {
        if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0, 1.f) &&
            !hovered_slot_name.empty()) {
            // Start creating
            is_link_creating = true;
            auto [src_n_name, src_idx] =
                    GetSrcName(hovered_slot_name, hovered_slot_idx, links);
            if (src_n_name.empty() || !is_hovered_slot_input) {
                // New link
                hovered_slot_name_prev = hovered_slot_name;
                hovered_slot_idx_prev = hovered_slot_idx;
                is_hovered_slot_input_prev = is_hovered_slot_input;
            } else {
                // Edit existing link
                hovered_slot_name_prev = src_n_name;
                hovered_slot_idx_prev = src_idx;
                is_hovered_slot_input_prev = false;
                issues->emplace_back([p_name, dst = hovered_slot_name,
                                      d_idx = hovered_slot_idx](auto pcm) {
                    (*pcm)[p_name].unlinkNode(dst, d_idx);
                });
            }
        }
    }
}
}  // namespace fase
