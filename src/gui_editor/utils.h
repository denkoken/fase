
#ifndef EDITOR_GUI_UTIL_H_20180917
#define EDITOR_GUI_UTIL_H_20180917

#include "../variable.h"

namespace fase {

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
    SwitchProject,
    RenameProject,
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

}  // namespace fase

#endif  // EDITOR_GUI_UTIL_H_20180917
