#ifndef EDITOR_GUI_UTIL_H_20180917
#define EDITOR_GUI_UTIL_H_20180917

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

#include "../core.h"
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
    Undo,
    Redo,
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
constexpr char POPUP_ADDING_SUB_PIPELINE[] = "adding sub pipeline";
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

inline void replace(const std::string& fr, const std::string& to,
                    std::string* str) {
    const size_t len = fr.length();
    for (size_t p = str->find(fr); p != std::string::npos; p = str->find(fr)) {
        *str = str->replace(p, len, to);
    }
}

inline std::string ToSnakeCase(const std::string& in) {
    std::string str = in;
    if (str[0] <= 'Z' && str[0] >= 'A') {
        str[0] -= 'A' - 'a';
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        replace({c}, "_" + std::string({char((c - 'A') + 'a')}), &str);
    }
    return str;
}

inline std::string GetEasyName(const std::string& func_name,
                               const FaseCore& core) {
    std::string node_name = ToSnakeCase(func_name);

    int i = 0;
    while (core.getNodes().count(node_name)) {
        node_name = ToSnakeCase(func_name) + std::to_string(++i);
    }
    return node_name;
}

}  // namespace guieditor
}  // namespace fase

#endif  // EDITOR_GUI_UTIL_H_20180917
