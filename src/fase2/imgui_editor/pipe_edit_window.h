
#ifndef PIPE_EDIT_WINDOW_H_20190305
#define PIPE_EDIT_WINDOW_H_20190305

#include <string>

#include "../common.h"
#include "../manager.h"
#include "../utils.h"

#include "imgui_commons.h"
#include "imgui_editor.h"
#include "links_view.h"

namespace fase {

using VarEditors = std::map<std::type_index, VarEditorWraped>;

class EditWindow {
public:
    bool draw(const std::string& p_name, const std::string& win_title,
              const CoreManager& cm, LabelWrapper label, Issues* issues,
              VarEditors* var_editors);

private:
    std::string selected_node_name;
    std::map<std::string, GuiNode> node_gui_utils;
    std::map<std::string, FunctionUtils> funcs;
    std::vector<std::string> f_names;
    std::string pipe_name;

    LinksView links_view;

    Report report;
    std::string err_message;

    // for allocate function popup
    Combo function_combo;
    // for rename/new node popup
    InputText new_node_name_it{64, ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll};

    // for edit input/output popup
    std::vector<InputText> input_arg_name_its;
    std::vector<InputText> output_arg_name_its;

    bool small_node_mode = false;

    std::map<std::string, EditWindow> children;

    void updateGuiNodeUtils(const PipelineAPI& cm);
    void updateMembers(const CoreManager& cm, const std::string& p_name);

    bool drawSmallNode(const std::string& n_name, const Node& node,
                       GuiNode& gui_node, const ImVec2& canvas_offset,
                       int base_channel, LabelWrapper& label, Issues* Issues,
                       VarEditors* var_editors);
    bool drawNormalNode(const std::string& n_name, const Node& node,
                        GuiNode& gui_node, const ImVec2& canvas_offset,
                        int base_channel, LabelWrapper& label, Issues* Issues,
                        VarEditors* var_editors);
    std::string drawNodes(const PipelineAPI& core_api, LabelWrapper label,
                          const int base_channel, Issues* issues,
                          VarEditors* var_editors);
    void drawNodeContextMenu(const std::string& hovered,
                             const PipelineAPI& core_api, LabelWrapper label,
                             Issues* issues);
    void drawEasyNodeGenarater(LabelWrapper label, Issues* issues);
    void drawCanvasContextMenu(const std::string& hovered, LabelWrapper label,
                               Issues* issues);
    void drawEditPannel(const PipelineAPI& core_api, LabelWrapper label,
                        Issues* issues, VarEditors* var_editors);
    void drawReportPannel(const PipelineAPI& core_api, LabelWrapper label,
                          Issues* issues);

    void drawChild(const CoreManager& cm, const std::string& child_p_name,
                   EditWindow&, LabelWrapper label, Issues* issues,
                   VarEditors* var_editors);

    void drawContent(const CoreManager& cm, LabelWrapper label, Issues* issues,
                     VarEditors* var_editors);
};

} // namespace fase

#endif // PIPE_EDIT_WINDOW_H_20190305
