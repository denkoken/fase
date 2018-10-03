
#include "view.h"

#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core_util.h"

namespace fase {

namespace {


class PreferenceMenu : public Content {
public:
    template <class... Args>
    PreferenceMenu(Args&&... args) : Content(args...) {}

    ~PreferenceMenu() {}

private:

    void main() {
        if (ImGui::MenuItem(label("Preferences"), NULL)) {
            state.popup_issue.emplace_back(POPUP_PREFERENCE);
        }
    }
};

// Draw button and pop up window for native code
class NativeCodeMenu : public Content {
public:
    template <class... Args>
    NativeCodeMenu(Args&&... args) : Content(args...) {}

    ~NativeCodeMenu() {}

private:

    void main() {
        if (ImGui::MenuItem(label("Show code"))) {
            // Open pop up window
            state.popup_issue.emplace_back(POPUP_NATIVE_CODE);
        }
    }
};

class ProjectMenu : public Content {
public:
    template <class... Args>
    ProjectMenu(Args&&... args) : Content(args...) {}

    ~ProjectMenu() {}

private:
    void main() {
        if (ImGui::MenuItem(label("Projects.."))) {
            state.popup_issue.emplace_back(POPUP_PROJECT);
        }
    }
};

constexpr char RUNNING_RESPONSE_ID[] = "RUNNING_RESPONSE_ID";

class RunPipelineMenu : public Content {
public:
    template <class... Args>
    RunPipelineMenu(Args&&... args) : Content(args...) {}

private:
    // Private status
    bool multi = false;
    bool report = true;

    bool getRunning() {
        bool f = false;
        bool ret = getResponse(RUNNING_RESPONSE_ID, &f);
        return ret && f;
    }

    void main() {
        bool is_running = getRunning();
        if (!is_running && ImGui::BeginMenu("Run")) {
            // Run once
            if (ImGui::MenuItem(label("Run")) && !is_running) {
                throwIssue(RUNNING_RESPONSE_ID, IssuePattern::BuildAndRun,
                           BuildAndRunInfo{multi, preference.another_th_run});
            }
            ImGui::Dummy(ImVec2(5, 0));  // Spacing
            // Run by loop
            if (!is_running) {
                if (ImGui::MenuItem(label("Run (loop)"))) {
                    // Start loop
                    throwIssue(
                            RUNNING_RESPONSE_ID, IssuePattern::BuildAndRunLoop,
                            BuildAndRunInfo{multi, preference.another_th_run});
                }
            }
            ImGui::MenuItem(label("Multi Build"), NULL, &multi);
            ImGui::MenuItem(label("Reporting"), NULL, &report);
            ImGui::MenuItem(label("Running on another thread"), NULL,
                            &preference.another_th_run);

            ImGui::EndMenu();
        } else if (is_running) {
            if (ImGui::MenuItem(label("Stop!"))) {
                // Stop
                throwIssue(RUNNING_RESPONSE_ID, IssuePattern::StopRunLoop, {});
            }
        }
    }
};

// Draw button and pop up window for node adding
class NodeAddingMenu : public Content {
public:
    template <class... Args>
    NodeAddingMenu(Args&&... args) : Content(args...) {}

    ~NodeAddingMenu() {}

private:

    void main() {
        if (ImGui::MenuItem(label("Add node"))) {
            state.popup_issue.emplace_back(POPUP_ADDING_NODE);
        }
    }
};

// Draw button and pop up window for native code
class AddInOutputMenu : public Content {
public:
    template <class... Args>
    AddInOutputMenu(Args&&... args) : Content(args...) {}

    ~AddInOutputMenu() {}
private:

    void main() {
        if (ImGui::MenuItem(label("Add input/output"))) {
            // Open pop up window
            state.popup_issue.emplace_back(POPUP_INPUT_OUTPUT);
        }
    }
};

class LayoutOptimizeMenu : public Content {
public:
    template <class... Args>
    LayoutOptimizeMenu(Args&&... args) : Content(args...) {}

    void main() {
        if (f) {
            preference.auto_layout = false;
            f = false;
        }
        if (ImGui::MenuItem(label("Optimize layout"))) {
            f = !preference.auto_layout;
            preference.auto_layout = true;
        }
    }

private:
    bool f = false;
};

/// Dummy class. Don't use this without a calling SetupContents().
class Footer {};

template <class Head, class... Tail>
void SetupContents(const FaseCore& core, LabelWrapper& label, GUIState& state,
                const TypeUtils& utils, std::function<void(Issue)> issue_f,
                std::vector<std::unique_ptr<Content>>* contents) {
    contents->emplace_back(
            std::make_unique<Head>(core, label, state, utils, issue_f));
    SetupContents<Tail...>(core, label, state, utils, issue_f, contents);
}

template <>
void SetupContents<Footer>(const FaseCore&, LabelWrapper&, GUIState&,
                        const TypeUtils&, std::function<void(Issue)>,
                        std::vector<std::unique_ptr<Content>>*) {}

}  // namespace

int Content::id_counter = 0;

void View::setupMenus(std::function<void(Issue)>&& issue_f) {
    // Setup Menu bar
    SetupContents<PreferenceMenu, ProjectMenu, NativeCodeMenu, NodeAddingMenu,
                  RunPipelineMenu, AddInOutputMenu, LayoutOptimizeMenu, Footer>(
            core, label, state, utils, issue_f, &menus);
}

}  // namespace fase
