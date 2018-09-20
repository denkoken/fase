
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
};

/// For AddNode issue
struct AddNodeInfo {
    std::string name;
    std::string func_repr;
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

struct Issue {
    std::string id;
    IssuePattern issue;
    Variable var;
};

}  // namespace fase

#endif  // EDITOR_GUI_UTIL_H_20180917
