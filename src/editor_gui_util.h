
#ifndef EDITOR_GUI_UTIL_H_20180917
#define EDITOR_GUI_UTIL_H_20180917

#include "variable.h"

namespace fase {

enum class IssuePattern {
    AddNode,
    DelNode,
    AddLink,
    DelLink,
    RenameNode,
    SetPriority,
    SetArgValue,
    Load,
    Save,
};

struct Issue {
    std::string id;
    IssuePattern issue;
    Variable var;
};

}  // namespace fase

#endif  // EDITOR_GUI_UTIL_H_20180917
