
#include "view.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "../core_util.h"

#if defined(_WIN64) || defined(_WIN32)
#else
#if __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#define FileSystem
using stdfs = std::experimental::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
#define FileSystem
using stdfs = std::filesystem;
#endif
#endif

namespace fase {

namespace {

void replace(const std::string& fr, const std::string& to, std::string* str) {
    const size_t len = fr.length();
    for (size_t p = str->find(fr); p != std::string::npos; p = str->find(fr)) {
        *str = str->replace(p, len, to);
    }
}

std::string ToSnakeCase(const std::string& in) {
    std::string str = in;
    if (str[0] <= 'Z' && str[0] >= 'A') {
        str[0] -= 'A' - 'a';
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        replace({c}, "_" + std::string({char((c - 'A') + 'a')}), &str);
    }
    return str;
}

// Additional ImGui components
bool IsItemActivePreviousFrame() {
    ImGuiContext* g = ImGui::GetCurrentContext();
    if (g->ActiveIdPreviousFrame) {
        return g->ActiveIdPreviousFrame == GImGui->CurrentWindow->DC.LastItemId;
    }
    return false;
}

bool IsKeyPressed(ImGuiKey_ key, bool repeat = true) {
    return ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[key], repeat);
};

bool IsKeyPressedOnItem(ImGuiKey_ key, bool repeat = true) {
    return IsItemActivePreviousFrame() && IsKeyPressed(key, repeat);
};

bool Combo(const char* label, int* curr_idx, std::vector<std::string>& vals) {
    if (vals.empty()) {
        return false;
    }
    return ImGui::Combo(
            label, curr_idx,
            [](void* vec, int idx, const char** out_text) {
                auto& vector = *static_cast<std::vector<std::string>*>(vec);
                if (idx < 0 || static_cast<int>(vector.size()) <= idx) {
                    return false;
                } else {
                    *out_text = vector.at(size_t(idx)).c_str();
                    return true;
                }
            },
            static_cast<void*>(&vals), int(vals.size()));
}

class PopupContent : public Content {
public:
    template <class... Args>
    PopupContent(std::string&& popup_name, bool menu, Args&&... args)
        : Content(args...), popup_name(popup_name), menu(menu) {}

    virtual ~PopupContent(){};

protected:
    virtual void layout() = 0;

    virtual bool init() {
        return true;
    }

private:
    std::string popup_name;
    bool menu;

    void main() {
        if (exists(state.popup_issue, popup_name)) {
            state.popup_issue.erase(std::find(std::begin(state.popup_issue),
                                              std::end(state.popup_issue),
                                              popup_name));
            if (init()) {
                ImGui::OpenPopup(label(popup_name.c_str()));
            }
        }
        bool opened = true;
        ImGuiWindowFlags option = ImGuiWindowFlags_AlwaysAutoResize;
        if (menu) {
            option |= ImGuiWindowFlags_MenuBar;
        }
        if (ImGui::BeginPopupModal(label(popup_name.c_str()), &opened,
                                   option)) {
            if (!opened || IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();  // Behavior of close button
            }

            layout();

            ImGui::Separator();

            if (ImGui::Button(label("Close"))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
};

class NativeCodePopup : public PopupContent {
public:
    template <class... Args>
    NativeCodePopup(Args&&... args)
        : PopupContent(POPUP_NATIVE_CODE, false, args...) {}

    ~NativeCodePopup() {}

private:
    std::string native_code;

    bool init() {
        native_code = GenNativeCode(core, utils);
        return true;
    }

    void layout() {
        ImGui::InputTextMultiline(label("##native code"),
                                  const_cast<char*>(native_code.c_str()),
                                  native_code.size(), ImVec2(500, 500),
                                  ImGuiInputTextFlags_ReadOnly);
    }
};

class AddingNodePopup : public PopupContent {
public:
    template <class... Args>
    AddingNodePopup(Args&&... args)
        : PopupContent(POPUP_ADDING_NODE, false, args...) {}

    ~AddingNodePopup() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    std::vector<std::string> func_reprs;
    char name_buf[64] = "";
    int curr_idx = 0;
    std::string error_msg;

    void updateFuncReprs(const std::map<std::string, Function>& functions) {
        func_reprs.clear();
        func_reprs.emplace_back("");
        for (auto it = functions.begin(); it != functions.end(); it++) {
            if (IsSpecialFuncName(it->first)) {
                continue;
            }
            func_reprs.push_back(it->first);
        }
    }

    bool init() {
        error_msg.clear();
        curr_idx = 0;

        // Extract function representations
        updateFuncReprs(core.getFunctions());
        return true;
    }

    void layout() {
        if (Combo(label("Function"), &curr_idx, func_reprs)) {
            std::string node_name = ToSnakeCase(func_reprs[size_t(curr_idx)]);

            int i = 0;
            while (core.getNodes().count(node_name)) {
                node_name = ToSnakeCase(func_reprs[size_t(curr_idx)]) +
                            std::to_string(++i);
            }
            strncpy(name_buf, node_name.c_str(), sizeof(name_buf));
        }

        bool throw_f = false;

        if (curr_idx) {
            ImGui::InputText(label("Node name (ID)"), name_buf,
                             sizeof(name_buf));
            throw_f = ImGui::Button(label("Add Node"));
        }

        bool success;
        if (throwIssue(IssuePattern::AddNode, throw_f,
                       AddNodeInfo{name_buf, func_reprs[size_t(curr_idx)]},
                       &success)) {
            if (success) {
                ImGui::CloseCurrentPopup();
            } else {
                error_msg = "Failed to create a new node";  // Failed
            }
        }

        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }
    }
};

class InputOutputPopup : public PopupContent {
public:
    template <class... Args>
    InputOutputPopup(Args&&... args)
        : PopupContent(POPUP_INPUT_OUTPUT, false, args...) {}

    ~InputOutputPopup() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);

    char in_name_buf[64] = "";
    char out_name_buf[64] = "";
    std::string error_msg;

    size_t selected_in_arg;
    size_t selected_out_arg;

    bool init() {
        selected_in_arg = size_t(-1);
        selected_out_arg = size_t(-1);
        return true;
    }

    void layout() {
        bool success = false;

        const Node& input_node = core.getNodes().at(InputNodeStr());
        const Function& input_func = core.getFunctions().at(InputFuncStr());

        const Node& output_node = core.getNodes().at(OutputNodeStr());
        const Function& output_func = core.getFunctions().at(OutputFuncStr());

        const ImVec2 panel_size(450,
                                120 + 25 * std::max(input_node.links.size(),
                                                    output_node.links.size()));
        ImGui::BeginChild(label("input_panel"), panel_size, true);

        for (size_t idx = 0; idx < input_func.arg_names.size(); idx++) {
            const std::string& arg_name = input_func.arg_names[idx];
            if (ImGui::Selectable(label(arg_name), selected_in_arg == idx)) {
                selected_in_arg = idx;
                selected_out_arg = size_t(-1);
            }
            ImGui::Spacing();
        }

        ImGui::Spacing();

        if (input_node.links.size() > 0 && selected_in_arg != size_t(-1)) {
            bool success_ = false;
            if (issueButton(IssuePattern::DelInput, selected_in_arg, &success_,
                            "Delelte input")) {
                if (success_) {
                    selected_in_arg = size_t(-1);
                }
            }
#if 0
            // TODO
            ImGui::SameLine();
            if (ImGui::Button(label("Reset Input"))) {
                const size_t arg_n =
                        core->getNodes().at(InputNodeStr()).links.size();
                for (size_t i = arg_n - 1; i < arg_n; i--) {
                    core->delInput(arg_n);
                }
                updater();
                ImGui::CloseCurrentPopup();
            }
#endif
            ImGui::Spacing();
        }
        ImGui::InputText(label("New Input Name"), in_name_buf,
                         sizeof(in_name_buf));

        if (issueButton(IssuePattern::AddInput, std::string(in_name_buf),
                        &success, "Make input")) {
            if (success) {
                error_msg = "";
                in_name_buf[0] = '\0';
            } else {
                error_msg = "Invalid Name";  // Failed
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild(label("output_panel"), panel_size, true);

        for (size_t idx = 0; idx < output_func.arg_names.size(); idx++) {
            const std::string& arg_name = output_func.arg_names[idx];
            if (ImGui::Selectable(label(arg_name), selected_out_arg == idx)) {
                selected_in_arg = size_t(-1);
                selected_out_arg = idx;
            }
            ImGui::Spacing();
        }
        ImGui::Spacing();

        if (output_node.links.size() > 0 && selected_out_arg != size_t(-1)) {
            bool success_ = false;
            if (issueButton(IssuePattern::DelOutput, selected_out_arg,
                            &success_, "Delelte output")) {
                if (success_) {
                    selected_out_arg = size_t(-1);
                }
            }
#if 0
            // TODO
            ImGui::SameLine();
            if (ImGui::Button(label("Reset Output"))) {
                const size_t arg_n =
                        core->getNodes().at(OutputNodeStr()).links.size();
                for (size_t i = arg_n - 1; i < arg_n; i--) {
                    core->delOutput(arg_n);
                }
                updater();
                ImGui::CloseCurrentPopup();
            }
#endif
            ImGui::Spacing();
        }

        ImGui::InputText(label("New Output Name"), out_name_buf,
                         sizeof(out_name_buf));
        if (issueButton(IssuePattern::AddOutput, std::string(out_name_buf),
                        &success, "Make output")) {
            if (success) {
                error_msg = "";
                out_name_buf[0] = '\0';
            } else {
                error_msg = "Invalid Name";  // Failed
            }
        }
        ImGui::EndChild();

        if (!error_msg.empty()) {
            ImGui::TextColored(ImVec4(255.f, 0.f, 0.f, 255.f), "%s",
                               error_msg.c_str());
        }
    }
};

class ProjectPopup : public PopupContent {
public:
    template <class... Args>
    ProjectPopup(Args&&... args)
        : PopupContent(POPUP_PROJECT, false, args...) {}

    ~ProjectPopup() {}

private:
    const ImVec4 ERROR_COLOR = ImVec4(255, 0, 0, 255);
    char load_filename_buf[128] = "fase_save.txt";
    char save_filename_buf[128];
    char project_name_buf[128] = "NewProject";
    char rename_buf[128] = "";
    int combo_idx = 0;
    std::string error_msg;
    int input_f = ImGuiInputTextFlags_AutoSelectAll |
                  ImGuiInputTextFlags_AlwaysInsertMode;

    enum struct Pattern {
        New,
        Save,
        Load,
        Rename,
        Switch,
    } pattern = Pattern::Load;

    bool init() {
        std::strcpy(save_filename_buf, core.getProjectName().c_str());
        combo_idx = 0;
        std::strcpy(rename_buf, core.getProjectName().c_str());
        return true;
    }

    void save() {
        ImGui::Text("Save Project");
        ImGui::Separator();

        ImGui::Text("File path :");
        ImGui::SameLine();
        ImGui::PushItemWidth(-150);
        ImGui::InputText(label(".project.txt"), save_filename_buf,
                         sizeof(save_filename_buf), input_f);
        ImGui::PopItemWidth();

        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }

        bool success;
        if (issueButton(IssuePattern::Save,
                        std::string(save_filename_buf) + ".project.txt",
                        &success, "Save")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Failed to save pipeline";  // Failed
            }
        }
    }

    void load() {
        ImGui::Text("Load Project");
        ImGui::Separator();

        ImGui::Text("File path :");
        ImGui::SameLine();

        ImGui::InputText(label(""), load_filename_buf,
                         sizeof(load_filename_buf), input_f);

        if (!error_msg.empty()) {
            ImGui::TextColored(ERROR_COLOR, "%s", error_msg.c_str());
        }

        bool success;
        if (issueButton(IssuePattern::Load, std::string(load_filename_buf),
                        &success, "Load")) {
            if (success) {
                ImGui::CloseCurrentPopup();
                error_msg = "";
            } else {
                error_msg = "Failed to load pipeline";  // Failed
            }
        }
#ifdef FileSystem
        stdfs::directory_iterator iter("."), end;
        for (; iter != end; iter++) {
            std::string path_str = iter->path().filename().generic_u8string();
            if (path_str.find(".project.txt") != std::string::npos) {
                std::cout << path_str << std::endl;
            }
        }
#endif
    }

    void new_project() {
        ImGui::Text("New Project");
        ImGui::Separator();

        ImGui::Text("Project Name :");
        ImGui::SameLine();
        ImGui::InputText(label(""), project_name_buf, sizeof(project_name_buf),
                         input_f);

        bool success;
        if (issueButton(IssuePattern::SwitchProject,
                        std::string(project_name_buf), &success,
                        "New Project")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void rename() {
        ImGui::Text("Rename Project");
        ImGui::Separator();

        ImGui::Text("Project New Name :");
        ImGui::SameLine();
        // Input elements
        ImGui::InputText(label(""), rename_buf, sizeof(rename_buf), input_f);

        bool success;
        if (issueButton(IssuePattern::RenameProject, std::string(rename_buf),
                        &success, "Rename")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void switch_project() {
        std::vector<std::string> projects = core.getProjects();
        Combo(label("project"), &combo_idx, projects);

        bool success;
        if (issueButton(IssuePattern::SwitchProject,
                        projects[size_t(combo_idx)], &success, "Switch")) {
            ImGui::CloseCurrentPopup();
        }
    }

    void layout() {
        // ImGui::BeginMenuBar();
        ImGui::BeginChild(label("project left panel"), ImVec2(150, 400));
        ImGui::Text("");
        ImGui::Separator();
        if (ImGui::Selectable(label("New Project"), pattern == Pattern::New)) {
            pattern = Pattern::New;
        }
        if (ImGui::Selectable(label("Save Project"),
                              pattern == Pattern::Save)) {
            pattern = Pattern::Save;
        }
        if (ImGui::Selectable(label("Load Project"),
                              pattern == Pattern::Load)) {
            pattern = Pattern::Load;
        }
#if 0
        if (ImGui::Selectable(label("Switch Project"), pattern == Pattern::Switch)) {
            pattern = Pattern::Switch;
        }
#endif
        if (ImGui::Selectable(label("Rename Project"),
                              pattern == Pattern::Rename)) {
            pattern = Pattern::Rename;
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild(label("project right panel"), ImVec2(500, 400));
        if (pattern == Pattern::Save) {
            save();
        } else if (pattern == Pattern::Load) {
            load();
        } else if (pattern == Pattern::New) {
            new_project();
        } else if (pattern == Pattern::Switch) {
            switch_project();
        } else if (pattern == Pattern::Rename) {
            rename();
        }
        ImGui::EndChild();
    }
};

class PreferencePopup : public PopupContent {
public:
    template <class... Args>
    PreferencePopup(Args&&... args)
        : PopupContent(POPUP_PREFERENCE, false, args...) {}

    ~PreferencePopup() {}

private:
    const std::string build_status_str = std::string("c++ version : ") +
                                         std::to_string(__cplusplus) +
                                         "\nc++ stdlib filesystem : " +
#ifdef FileSystem
                                         "True" +
#else
                                         "False" +
#endif
                                         "\nconstexpr if : " +
#ifdef __cpp_if_constexpr
                                         "True" +
#else
                                         "False" +
#endif
                                         "\ninline variables : " +
#ifdef __cpp_inline_variables
                                         "True" +
#else
                                         "False" +
#endif
                                         "";

    void layout() {
        {  // Node edit Settings
            constexpr int v_min = std::numeric_limits<int>::min();
            constexpr int v_max = std::numeric_limits<int>::max();
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("priority min"), &preference.priority_min,
                           v_speed, v_min, preference.priority_max);
            ImGui::DragInt(label("priority max"), &preference.priority_max,
                           v_speed, preference.priority_min, v_max);
        }

        ImGui::Separator();

        {  // Node view Settings
            ImGui::Checkbox(label("Auto Layout Canvas"),
                            &preference.auto_layout);
            ImGui::Checkbox(label("Simple Node Boxes"),
                            &preference.is_simple_node_box);

            constexpr int v_min = 3;
            constexpr int v_max = 64;
            constexpr float v_speed = 0.5;
            ImGui::DragInt(label("argument char max"),
                           &preference.max_arg_name_chars, v_speed, v_min,
                           v_max);
        }

        ImGui::Separator();

        {  // Panel Settings
            ImGui::Checkbox(label("Edit Panel View"),
                            &preference.enable_edit_panel);
            ImGui::Checkbox(label("Node List Panel View"),
                            &preference.enable_node_list_panel);
            constexpr int v_min = 0;
            constexpr int v_max = std::numeric_limits<int>::max();
            constexpr float v_speed = 0.5;

            ImGui::DragInt(label("node list panel size"),
                           &preference.node_list_panel_size, v_speed, v_min,
                           v_max);
            ImGui::DragInt(label("edit panel size"),
                           &preference.edit_panel_size, v_speed, v_min, v_max);
        }

        ImGui::Separator();

        ImGui::Text("Build Status");
        // show Build Status
        ImGui::InputTextMultiline(label("##build status"),
                                  const_cast<char*>(build_status_str.c_str()),
                                  build_status_str.size(), ImVec2(-1, 0),
                                  ImGuiInputTextFlags_ReadOnly);
    }
};

class RenameNodePopup : public PopupContent {
public:
    template <class... Args>
    RenameNodePopup(Args&&... args)
        : PopupContent(POPUP_RENAME_NODE, false, args...) {}

    ~RenameNodePopup() {}

private:
    std::string renaming_node;
    char new_node_name[64] = "";

    bool init() {
        if (state.selected_nodes.empty()) {
            return false;
        }
        renaming_node = state.selected_nodes[0];
        return true;
    }
    void layout() {
        ImGui::InputText(label("New node name (ID)"), new_node_name,
                         sizeof(new_node_name));

        bool success;
        if (issueButton(IssuePattern::RenameNode,
                        RenameNodeInfo{renaming_node, new_node_name}, &success,
                        "OK")) {
            ImGui::CloseCurrentPopup();
        }
    }
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

void View::setupPopups(std::function<void(Issue)>&& issue_f) {
    SetupContents<PreferencePopup, NativeCodePopup, AddingNodePopup,
                  InputOutputPopup, ProjectPopup, RenameNodePopup, Footer>(
            core, label, state, utils, issue_f, &popups);
}

}  // namespace fase
