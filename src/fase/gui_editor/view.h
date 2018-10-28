#ifndef EDITOR_GUI_VIEW_H_20180917
#define EDITOR_GUI_VIEW_H_20180917

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core.h"
#include "../type_utils.h"
#include "preference.h"
#include "utils.h"

namespace fase {
namespace guieditor {

constexpr char REPORT_RESPONSE_ID[] = "REPORT_RESPONSE_ID";
constexpr char RUNNING_ERROR_RESPONSE_ID[] = "RUNNING_ERROR_RESPONSE_ID";

// Label wrapper for suffix
class LabelWrapper {
public:
    void setSuffix(const std::string& s) {
        suffix = s;
    }
    void addSuffix(const std::string& s) {
        suffix += s;
    }
    const char* operator()(const std::string& label) {
        last_label = label + suffix;
        return last_label.c_str();
    }

private:
    std::string suffix;
    std::string last_label;  // temporary storage to return char*
};

struct GUIState {
    GUIPreference& preference;

    std::vector<std::string> selected_nodes;
    std::string hovered_node_name;
    std::vector<std::string> node_order;

    std::vector<std::string> popup_issue;

    bool is_running = false;
};

class Content {
public:
    Content(const FaseCore& core_, LabelWrapper& label_, GUIState& state_,
            const TypeUtils& utils_, const std::function<void(Issue)>& issue_f_)
        : core(core_),
          utils(utils_),
          label(label_),
          state(state_),
          preference(state_.preference),
          id(id_counter++),
          issue_f(issue_f_) {}
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

using VarEditorWraped = std::function<Variable(const char*, const Variable&)>;

class View {
public:
    View(const FaseCore&, const TypeUtils&,
         const std::map<const std::type_info*, VarEditorWraped>&);
    ~View();

    std::vector<Issue> draw(const std::string& win_title,
                            const std::string& label_suffix,
                            const std::map<std::string, Variable>& response);

private:
    const FaseCore& core;
    const TypeUtils& utils;
    const std::map<const std::type_info*, VarEditorWraped>& var_editors;

    // this will be save/load preferences to/from buffer file.
    GUIPreferenceManager preference_manager;

    LabelWrapper label;
    GUIState state;

    std::vector<Issue> issues;

    // GUI Contents
    std::vector<std::unique_ptr<Content>> menus;   // Menu bar
    std::vector<std::unique_ptr<Content>> popups;  // popup menus
    std::unique_ptr<Content> node_list;            // left panel
    std::unique_ptr<Content> canvas;               // right panel
    std::unique_ptr<Content> args_editor;          // center panel
    std::unique_ptr<Content> report_window;

    void setupMenus(std::function<void(Issue)>&&);
    void setupPopups(std::function<void(Issue)>&&);
    void updateState(const std::map<std::string, Variable>& resp);

    int privious_core_version = -1;
};

}  // namespace guieditor
}  // namespace fase

#endif  // EDITOR_GUI_VIEW_H_20180917
