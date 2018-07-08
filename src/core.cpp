#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

namespace fase {

namespace {

template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end(); it++) {
        dst_set.emplace(it->first);
    }
}

std::string FindRunnableNode(const std::set<std::string>& unused_node_names,
                             const std::map<std::string, Node>& nodes) {
    // Find runnable function node
    for (auto& node_name : unused_node_names) {
        const Node& node = nodes.at(node_name);

        bool runnable = true;
        size_t arg_idx = 0;
        for (auto& link : node.links) {
            if (link.node_name.empty()) {
                // Case 1: Create default argument
            } else {
                // Case 2: Use output variable
                // Is linked node created?
                if (exists(unused_node_names, link.node_name)) {
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

class RunnableNodeStack {
public:
    explicit RunnableNodeStack(const std::map<std::string, Node>* nodes_p)
        : nodes(nodes_p) {
        // Mark all node names as unused
        extractKeys(*nodes, unused_node_names);
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

std::string genVarName(const std::string& node_name, const size_t arg_idx) {
    std::stringstream var_name_ss;
    var_name_ss << node_name << "_" << arg_idx;
    return var_name_ss.str();
}

std::string removeReprRef(const std::string& type_repr) {
    auto idx = type_repr.rfind('&');
    if (idx == std::string::npos) {
        return type_repr;
    } else {
        return type_repr.substr(0, idx);
    }
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

}  // anonymous namespace

bool FaseCore::addNode(const std::string& name, const std::string& func_repr) {
    if (name.empty()) {
        return false;
    }
    // check defined function name.
    if (!exists(functions, func_repr)) {
        return false;
    }

    // check uniqueness of name.
    if (exists(nodes, name)) {
        return false;
    }

    // Make clone of default variables
    std::vector<Variable> arg_values;
    for (auto& v : functions[func_repr].default_arg_values) {
        arg_values.push_back(v.clone());
    }

    // Register node (arg_values are copied from function's default_arg_values)
    const size_t n_args = functions[func_repr].arg_type_reprs.size();
    nodes[name] = {.func_repr = func_repr,
                   .links = std::vector<Link>(n_args),
                   .arg_reprs = functions[func_repr].default_arg_reprs,
                   .arg_values = arg_values};

    return true;
}

void FaseCore::delNode(const std::string& node_name) noexcept {
    // Remove connected links
    for (auto it = nodes.begin(); it != nodes.end(); it++) {
        Node& node = it->second;
        for (Link& link : node.links) {
            if (link.node_name == node_name) {
                // Remove
                link = {};
            }
        }
    }

    // Remove node
    nodes.erase(node_name);
}

bool FaseCore::addLink(const std::string& src_node_name,
                       const size_t& src_arg_idx,
                       const std::string& dst_node_name,
                       const size_t& dst_arg_idx) {
    if (!exists(nodes, src_node_name) || !exists(nodes, dst_node_name)) {
        return false;
    }
    if (nodes[dst_node_name].links.size() <= dst_arg_idx ||
        nodes[src_node_name].links.size() <= src_arg_idx) {
        return false;
    }

    // Check looping link
    if (src_node_name == dst_node_name) {
        return false;
    }

    // Check inverse link
    if (nodes[src_node_name].links[src_arg_idx].node_name == dst_node_name &&
        nodes[src_node_name].links[src_arg_idx].arg_idx == dst_arg_idx) {
        return false;
    }

    // Check types
    if (!nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                nodes[src_node_name].arg_values[src_arg_idx])) {
        std::cerr << "Invalid types to create link" << std::endl;
        return false;
    }

    // Check link existence
    if (!nodes[dst_node_name].links[dst_arg_idx].node_name.empty()) {
        return false;
    }

    // Register
    nodes[dst_node_name].links[dst_arg_idx] = {src_node_name, src_arg_idx};

    // Test for loop link
    RunnableNodeStack runnable_nodes_stack(&nodes);
    while (!runnable_nodes_stack.pop().empty())
        ;
    if (!runnable_nodes_stack.empty()) {
        // Revert registration
        nodes[dst_node_name].links[dst_arg_idx] = {};
        return false;
    }

    return true;
}

void FaseCore::delLink(const std::string& dst_node_name,
                       const size_t& dst_arg_idx) {
    if (!exists(nodes, dst_node_name)) {
        return;
    }
    if (nodes[dst_node_name].links.size() <= dst_arg_idx) {
        return;
    }

    // Delete
    nodes[dst_node_name].links[dst_arg_idx] = {};
}

bool FaseCore::setNodeArg(const std::string& node_name, const size_t arg_idx,
                          const std::string& arg_repr, Variable arg_val) {
    if (!exists(nodes, node_name)) {
        return false;
    }
    Node& node = nodes[node_name];
    if (node.links.size() <= arg_idx) {
        return false;
    }

    // Check input type
    if (!node.arg_values[arg_idx].isSameType(arg_val)) {
        std::cerr << "Invalid input type to set node argument" << std::endl;
        return false;
    }

    // Register
    node.arg_reprs[arg_idx] = arg_repr;
    node.arg_values[arg_idx] = arg_val;
    return true;
}

void FaseCore::clearNodeArg(const std::string& node_name,
                            const size_t arg_idx) {
    if (!exists(nodes, node_name)) {
        return;
    }
    Node& node = nodes[node_name];
    if (node.links.size() <= arg_idx) {
        return;
    }

    // Clear (overwrite by default values)
    const std::string& func_repr = node.func_repr;
    node.arg_reprs[arg_idx] = functions[func_repr].default_arg_reprs[arg_idx];
    node.arg_values[arg_idx] = functions[func_repr].default_arg_values[arg_idx];
}

const std::map<std::string, Node>& FaseCore::getNodes() {
    return nodes;
}

const std::map<std::string, Function>& FaseCore::getFunctions() {
    return functions;
}

bool FaseCore::build() {
    pipeline.clear();
    output_variables.clear();

    // Stack for finding runnable node
    RunnableNodeStack runnable_nodes_stack(&nodes);

    while (true) {
        // Find runnable node
        const std::string node_name = runnable_nodes_stack.pop();
        if (node_name.empty()) {
            break;
        }

        Node& node = nodes[node_name];
        const size_t n_args = node.links.size();

        // Set output variable
        assert(output_variables[node_name].empty());
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            if (link.node_name.empty()) {
                // Case 1: Create default argument
                const Variable& v = node.arg_values[arg_idx];
                output_variables[node_name].push_back(v);
            } else {
                // Case 2: Use output variable
                Variable& v = output_variables.at(link.node_name)[link.arg_idx];
                output_variables[node_name].push_back(v);
            }
        }

        // Collect binding variables
        std::vector<Variable*> bound_variables;
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            bound_variables.push_back(&output_variables[node_name][arg_idx]);
        }
        // Build
        Function& func = functions[node.func_repr];
        pipeline.push_back(func.builder->build(bound_variables));
    }

    return true;
}

bool FaseCore::run() {
    for (auto& f : pipeline) {
        f();
    }
    return true;
}

std::string FaseCore::genNativeCode(const std::string& entry_name,
                                    const std::string& indent) {
    std::stringstream native_code;

    // Stack for finding runnable node
    RunnableNodeStack runnable_nodes_stack(&nodes);

    if (!entry_name.empty()) {
        native_code << "void " << entry_name << "() {" << std::endl;
    }

    while (true) {
        // Find runnable node
        const std::string node_name = runnable_nodes_stack.pop();
        if (node_name.empty()) {
            break;
        }

        Node& node = nodes[node_name];
        const size_t n_args = node.links.size();

        // Add comment
        native_code << indent;
        native_code << "// " << node.func_repr << " [" << node_name << "]"
                    << std::endl;

        // Argument representations
        const std::vector<std::string>& arg_type_reprs =
                functions[node.func_repr].arg_type_reprs;
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

    if (!entry_name.empty()) {
        native_code << "}";
    }

    return native_code.str();
}

}  // namespace fase
