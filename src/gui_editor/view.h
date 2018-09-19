
#ifndef EDITOR_GUI_VIEW_H_20180917
#define EDITOR_GUI_VIEW_H_20180917

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core.h"
#include "utils.h"
#include "../type_utils.h"

namespace fase {

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

struct GUIState {
    std::vector<std::string> selected_nodes;
    std::string hovered_node_name;
    std::vector<std::string> node_order;

    GUIPreference preference;
};

class Content {
public:
    Content(const FaseCore& core, LabelWrapper& label,
            GUIState& state, const TypeUtils& utils, const std::function<void(Issue)>& issue_f)
        : core(core),
          utils(utils),
          label(label),
          state(state),
          preference(state.preference),
          id(id_counter++),
          issue_f(issue_f) {}
    virtual ~Content() = 0;

    void draw(const std::map<std::string, Variable>& response) {
        response_p = &response;
        main();
    }

protected:
    const FaseCore& core;
    const TypeUtils& utils;

    // Reference to the parent's
    LabelWrapper& label;
    GUIState& state;
    GUIPreference& preference;

    template <typename Ret, typename T>
    bool throwIssue(const IssuePattern& pattern, bool is_throw, T&& var,
                    Ret* ret, std::string res_id_footer = "") {
        std::string res_id = std::to_string(id) + "::" + res_id_footer;
        if (is_throw) {
            Issue issue = {
                    .id = res_id,
                    .issue = pattern,
                    .var = var,
            };
            issue_f(std::move(issue));
        }

        if (exists(*response_p, res_id)) {
            *ret = *response_p->at(res_id).getReader<Ret>();
            return true;
        }
        return false;
    }

    template <typename Ret, typename T, typename... Args>
    bool issueButton(const IssuePattern& pattern, T&& var, Ret* ret,
                     const char* text, Args&&... args) {
        return throwIssue(pattern, ImGui::Button(label(text), args...), var,
                          ret, text);
    }

    virtual void main() = 0;

private:
    static int id_counter;

    const int id;

    const std::map<std::string, Variable>* response_p;
    std::function<void(Issue)> issue_f;
};

class NodeListView;
class NodeCanvasView;

class View {
public:
    View(const FaseCore&, const TypeUtils&);
    ~View();

    std::vector<Issue> draw(const std::string& win_title,
                            const std::string& label_suffix,
                            const std::map<std::string, Variable>& response);

private:
    const FaseCore& core;
    const TypeUtils& utils;

    LabelWrapper label;
    GUIState state;

    std::vector<Issue> issues;

    // GUI Contents
    std::vector<std::unique_ptr<Content>> menus;  // Menu bar
    std::unique_ptr<NodeListView> node_list;
    std::unique_ptr<NodeCanvasView> canvas;

    void setupMenus(std::function<void(Issue)>&&);
};

}  // namespace fase

#endif  // EDITOR_GUI_VIEW_H_20180917
