
#ifndef LINKS_VIEW_H_20190308
#define LINKS_VIEW_H_20190308

#include <cmath>
#include <string>

#include <imgui.h>

#include "imgui_commons.h"

namespace fase {

class LinksView {
public:
    void draw(const std::map<std::string, Node>& nodes,
              const std::vector<Link>& links, const std::string& p_name,
              std::map<std::string, GuiNode>& node_gui_utils, Issues* issues);

private:
    constexpr static ImU32 LINK_COLOR = IM_COL32(200, 200, 100, 255);
    constexpr static float SLOT_HOVER_RADIUS = 8.f;
    // const float ARROW_BEZIER_SIZE = 80.f;
    constexpr static float ARROW_HEAD_SIZE = 15.f;
    const float ARROW_HEAD_X_OFFSET = -ARROW_HEAD_SIZE * std::sqrt(3.f) * 0.5f;

    std::string hovered_slot_name_prev = "";
    std::string hovered_slot_name = "";
    std::size_t hovered_slot_idx_prev = 0;
    std::size_t hovered_slot_idx = 0;
    bool is_hovered_slot_input_prev = false;
    bool is_hovered_slot_input = false;
    bool is_link_creating = false;

    void drawLink(const ImVec2& s_pos, const ImVec2& d_pos,
                  const std::size_t& d_id);
};
}  // namespace fase

#endif  // LINKS_VIEW_H_20190308
