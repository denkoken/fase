
#ifndef EDITOR_GUI_VIEW_H_20180917
#define EDITOR_GUI_VIEW_H_20180917

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core.h"
#include "../type_utils.h"
#include "utils.h"

namespace fase {

constexpr char REPORT_RESPONSE_ID[] = "REPORT_RESPONSE_ID";

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
    bool is_simple_node_box = false;
    int max_arg_name_chars = 16;
    bool enable_edit_panel = false;
    bool enable_node_list_panel = true;

    int node_list_panel_size = 150;
    int edit_panel_size = 200;
};

struct GUIState {
    std::vector<std::string> selected_nodes;
    std::string hovered_node_name;
    std::vector<std::string> node_order;

    GUIPreference preference;
};

class Content {
public:
    Content(const FaseCore& core, LabelWrapper& label, GUIState& state,
            const TypeUtils& utils, const std::function<void(Issue)>& issue_f)
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

    void throwIssue(std::string&& res_id, IssuePattern&& pattern,
                    Variable&& var) {
        issue_f(Issue{res_id, pattern, var});
    }

    template <typename T>
    bool getResponse(const std::string& res_id, T* ret) {
        if (response_p->count(res_id)) {
            *ret = *response_p->at(res_id).getReader<T>();
            return true;
        }
        return false;
    }

    template <typename T, typename Ret = int>
    bool throwIssue(const IssuePattern& pattern, bool is_throw, T&& var,
                    Ret* ret = nullptr, std::string res_id_footer = "") {
        std::string res_id = std::to_string(id) + "::" + res_id_footer;
        if (is_throw) {
            issue_f(Issue{res_id, pattern, var});
        }

        if (ret == nullptr) {
            return false;
        }

        if (exists(*response_p, res_id)) {
            *ret = *response_p->at(res_id).getReader<Ret>();
            return true;
        }
        return false;
    }

    template <typename T, typename Ret, typename... Args>
    bool issueButton(const IssuePattern& pattern, T&& var, Ret* ret,
                     const char* text, Args&&... args) {
        return throwIssue(pattern, ImGui::Button(label(text), args...), var,
                          ret, text);
    }

    template <class ChildContent>
    void drawChild(ChildContent& child) {
        child.draw(*response_p);
    }

    virtual void main() = 0;

private:
    static int id_counter;

    const int id;

    const std::map<std::string, Variable>* response_p;
    std::function<void(Issue)> issue_f;
};

using VarEditor = std::function<Variable(const char*, const Variable&)>;

class View {
public:
    View(const FaseCore&, const TypeUtils&,
         const std::map<const std::type_info*, VarEditor>&);
    ~View();

    std::vector<Issue> draw(const std::string& win_title,
                            const std::string& label_suffix,
                            const std::map<std::string, Variable>& response);

private:
    const FaseCore& core;
    const TypeUtils& utils;
    const std::map<const std::type_info*, VarEditor>& var_editors;

    LabelWrapper label;
    GUIState state;

    std::vector<Issue> issues;

    // GUI Contents
    std::vector<std::unique_ptr<Content>> menus;  // Menu bar
    std::unique_ptr<Content> node_list;
    std::unique_ptr<Content> canvas;
    std::unique_ptr<Content> args_editor;
    std::unique_ptr<Content> report_window;

    void setupMenus(std::function<void(Issue)>&&);
    void updateState();
};

}  // namespace fase

#endif  // EDITOR_GUI_VIEW_H_20180917
