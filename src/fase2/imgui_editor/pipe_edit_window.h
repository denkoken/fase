
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
    InputText new_node_name_it{64, ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_AutoSelectAll};
    std::string selected_node_name;
    std::map<std::string, GuiNode> node_gui_utils;
    Combo function_combo;
    std::map<std::string, FunctionUtils> funcs;
    std::string pipe_name;

    LinksView links_view;

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
