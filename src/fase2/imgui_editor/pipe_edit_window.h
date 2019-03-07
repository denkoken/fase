
#ifndef PIPE_EDIT_WINDOW_H_20190305
#define PIPE_EDIT_WINDOW_H_20190305

#include <deque>
#include <string>

#include "../common.h"
#include "../manager.h"
#include "../utils.h"

#include "imgui_commons.h"
#include "imgui_editor.h"

namespace fase {

struct GuiNode {
    ImVec2 pos;
    ImVec2 size;
    std::vector<ImVec2> arg_poses;
    std::vector<char> arg_inp_hovered;
    std::vector<char> arg_out_hovered;
    size_t id = size_t(-1);

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

using Issues = std::deque<std::function<void(CoreManager*)>>;
using VarEditors = std::map<std::type_index, VarEditorWraped>;

class EditWindow {
public:
    bool draw(const std::string& p_name, const std::string& win_title,
              const CoreManager& cm, LabelWrapper label, Issues* issues,
              VarEditors* var_editors);

private:
    InputText new_node_name_it{64, ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll};
    std::string selected_node_name;
    std::map<std::string, GuiNode> node_gui_utils;
    Combo function_combo;
    std::map<std::string, FunctionUtils> funcs;
    std::string pipe_name;

    void updateGuiNodeUtils(const PipelineAPI& cm);
    void updateMembers(const CoreManager& cm, const std::string& p_name);

    std::string drawNodes(const PipelineAPI& core_api, LabelWrapper label,
                          Issues* issues, VarEditors* var_editors);
    void drawNodeContextMenu(const std::string& hovered,
                             const PipelineAPI& core_api, LabelWrapper label,
                             Issues* issues);
    void drawCanvasContextMenu(const std::string& hovered, LabelWrapper label,
                               Issues* issues);
    void drawCanvasPannel(const PipelineAPI& core_api, LabelWrapper label,
                          Issues* issues, VarEditors* var_editors);
};

}  // namespace fase

#endif  // PIPE_EDIT_WINDOW_H_20190305
