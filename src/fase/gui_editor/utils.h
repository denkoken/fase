#ifndef EDITOR_GUI_UTIL_H_20180917
#define EDITOR_GUI_UTIL_H_20180917

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "../variable.h"

namespace fase {
namespace guieditor {

enum class IssuePattern {
    AddNode,
    DelNode,
    RenameNode,
    AddLink,
    DelLink,
    AddInput,
    DelInput,
    AddOutput,
    DelOutput,
    SetPriority,
    SetArgValue,
    Load,
    Save,
    BuildAndRun,
    BuildAndRunLoop,
    StopRunLoop,
    SwitchPipeline,
    RenamePipeline,
    MakeSubPipeline,
    ImportSubPipeline,
};

/// For AddNode issue
struct AddNodeInfo {
    std::string name;
    std::string func_repr;
};

struct SetPriorityInfo {
    std::string nodename;
    int priority;
};

struct AddLinkInfo {
    std::string src_nodename;
    size_t src_idx;
    std::string dst_nodename;
    size_t dst_idx;
};

struct DelLinkInfo {
    std::string src_nodename;
    size_t src_idx;
};

struct SetArgValueInfo {
    std::string node_name;
    size_t arg_idx;
    std::string arg_repr;
    Variable var;
};

struct RenameNodeInfo {
    std::string old_name;
    std::string new_name;
};

struct BuildAndRunInfo {
    bool multi_th_build;
    bool another_th_run;
};

struct ImportSubPipelineInfo {
    std::string filename;
    std::string target_name;
};

struct Issue {
    std::string id;
    IssuePattern issue;
    Variable var;
};

constexpr char POPUP_ADDING_NODE[] = "adding node popup";
constexpr char POPUP_NATIVE_CODE[] = "genarate native code popup";
constexpr char POPUP_INPUT_OUTPUT[] = "in/output popup";
constexpr char POPUP_PROJECT[] = "project popup";
constexpr char POPUP_PREFERENCE[] = "preference popup";
constexpr char POPUP_RENAME_NODE[] = "rename node popup";

inline bool Combo(const char* label, int* curr_idx,
                  std::vector<std::string>& vals) {
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

}  // namespace guieditor
}  // namespace fase

#endif  // EDITOR_GUI_UTIL_H_20180917
