
#ifndef EDITOR_GUI_VIEW_H_20180917
#define EDITOR_GUI_VIEW_H_20180917

#include <vector>

#include "core.h"
#include "editor_gui_util.h"
#include "type_utils.h"

namespace fase {

struct GUIPreference {
    bool auto_layout = false;
    int priority_min = -10;
    int priority_max = 10;
};

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

class Menu {
public:
    Menu(const FaseCore& core, LabelWrapper& label, GUIPreference& preference,
         const TypeUtils& utils)
        : core(core), label(label), preference(preference), utils(utils) {}
    virtual ~Menu() = 0;

    virtual std::vector<Issue> draw(
            const std::map<std::string, Variable>& response) = 0;

protected:
    const FaseCore& core;
    // Reference to the parent's
    LabelWrapper& label;
    GUIPreference& preference;
    const TypeUtils& utils;
};

class View {
public:
    View(const FaseCore&, const TypeUtils&);

    std::vector<Issue> draw(const std::string& win_title,
                            const std::string& label_suffix,
                            const std::map<std::string, Variable>& response);

private:
    std::vector<std::unique_ptr<Menu>> menus;  // Menu bar

    const FaseCore& core;
    LabelWrapper label;
    GUIPreference preference;
    const TypeUtils& utils;

    void setupMenus();
};

}  // namespace fase

#endif  // EDITOR_GUI_VIEW_H_20180917
