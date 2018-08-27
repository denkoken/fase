
#include "core_util.h"

#include <sstream>

namespace {

template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end(); it++) {
        dst_set.emplace(it->first);
    }
}

std::string FindRunnableNode(const std::set<std::string>& unused_node_names,
                             const std::map<std::string, fase::Node>& nodes) {
    // Find runnable function node
    for (auto& node_name : unused_node_names) {
        const fase::Node& node = nodes.at(node_name);

        bool runnable = true;
        size_t arg_idx = 0;
        for (auto& link : node.links) {
            if (link.node_name.empty()) {
                // Case 1: Create default argument
            } else {
                // Case 2: Use output variable
                // Is linked node created?
                if (fase::exists(unused_node_names, link.node_name)) {
                    runnable = false;
                    break;
                }
            }
            arg_idx++;
        }

        if (runnable) {
            return node_name;
        }
    }

    return std::string();
}

std::string genVarName(const std::string& node_name, const size_t arg_idx) {
    std::stringstream var_name_ss;
    var_name_ss << node_name << "_" << arg_idx;
    return var_name_ss.str();
}

std::string removeReprConst(const std::string& type_repr) {
    const std::string const_str = "const ";
    auto idx = type_repr.rfind(const_str);
    if (idx == std::string::npos) {
        return type_repr;
    } else {
        return type_repr.substr(idx + const_str.size());
    }
}

std::string removeReprRef(const std::string& type_repr) {
    auto idx = type_repr.rfind('&');
    if (idx == std::string::npos) {
        return type_repr;
    } else {
        return type_repr.substr(0, idx);
    }
}

std::string genVarDeclaration(const std::string& type_repr,
                              const std::string& val_repr,
                              const std::string& name) {
    // Add declaration code (Remove reference for declaration)
    std::stringstream ss;
    ss << removeReprConst(removeReprRef(type_repr)) << " " << name;
    if (!val_repr.empty()) {
        // Add variable initialization
        ss << " = " << val_repr;
    }
    ss << ";" << std::endl;
    return ss.str();
}

std::string genFunctionCall(const std::string& func_repr,
                            const std::vector<std::string>& var_names) {
    // Add declaration code (Remove reference for declaration)
    std::stringstream ss;
    ss << func_repr << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << var_names[i];
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << std::endl;
    return ss.str();
}

}  // namespace

namespace fase {

class RunnableNodeStack {
public:
    explicit RunnableNodeStack(const std::map<std::string, Node>& nodes_)
        : nodes(&nodes_) {
        // Mark all node names as unused
        extractKeys(nodes_, unused_node_names);
    }

    std::string pop() {
        // Find runnable node by checking link dependency
        const std::string& node_name =
                FindRunnableNode(unused_node_names, *nodes);
        if (!node_name.empty()) {
            unused_node_names.erase(node_name);  // Mark as used
        }
        return node_name;
    }

    bool empty() {
        return unused_node_names.empty();
    }

private:
    std::set<std::string> unused_node_names;
    const std::map<std::string, Node>* nodes;
};

std::string GenNativeCode(const FaseCore& core, const std::string& entry_name,
                          const std::string& indent) {
    std::stringstream native_code;

    // Stack for finding runnable node
    RunnableNodeStack runnable_nodes_stack(core.getNodes());

    if (!entry_name.empty()) {
        native_code << "void " << entry_name << "() {" << std::endl;
    }

    while (true) {
        // Find runnable node
        const std::string node_name = runnable_nodes_stack.pop();
        if (node_name.empty()) {
            break;
        }

        const Node& node = core.getNodes().at(node_name);
        const size_t n_args = node.links.size();

        // Add comment
        native_code << indent;
        native_code << "// " << node.func_repr << " [" << node_name << "]"
                    << std::endl;

        // Argument representations
        const std::vector<std::string>& arg_type_reprs =
                core.getFunctions().at(node.func_repr).arg_type_reprs;
        const std::vector<std::string>& arg_reprs = node.arg_reprs;

        // Collect argument names
        std::vector<std::string> var_names;
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            if (link.node_name.empty()) {
                // Case 1: Create default argument
                var_names.push_back(genVarName(node_name, arg_idx));
                // Add declaration code
                native_code << indent;
                native_code << genVarDeclaration(arg_type_reprs[arg_idx],
                                                 arg_reprs[arg_idx],
                                                 var_names.back());
            } else {
                // Case 2: Use output variable
                var_names.push_back(genVarName(link.node_name, link.arg_idx));
            }
        }

        // Add function call
        native_code << indent;
        native_code << genFunctionCall(node.func_repr, var_names);
    }
    assert(runnable_nodes_stack.empty());

    if (!entry_name.empty()) {
        native_code << "}";
    }

    return native_code.str();
}

}  // namespace fase