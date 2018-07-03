#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>

namespace fase {

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end(); it++) {
        dst_set.emplace(it->first);
    }
}

std::string FindRunnableNode(
        const std::set<std::string>& unused_node_names,
        const std::map<std::string, Node>& nodes,
        const std::map<std::string, std::vector<Variable>>& output_variables) {
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
                if (!exists(output_variables, link.node_name)) {
                    runnable = false;
                    break;
                }
                // Is argument index valid?
                const size_t n_args =
                        output_variables.at(link.node_name).size();
                if (link.arg_idx < 0 || n_args <= link.arg_idx) {
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

}  // anonymous namespace

bool FaseCore::makeNode(const std::string& name, const std::string& func_repr) {
    // check defined function name.
    if (!exists(functions, func_repr)) {
        return false;
    }

    // check uniqueness of name.
    if (exists(nodes, name)) {
        return false;
    }

    // Register node (arg_values are copied from function's default_arg_values)
    const size_t n_args = functions[func_repr].arg_reprs.size();
    nodes[name] = {.func_repr = func_repr,
                   .links = std::vector<Link>(n_args),
                   .arg_values = functions[func_repr].default_arg_values};

    return true;
}

void FaseCore::delNode(const std::string& name) noexcept {
    nodes.erase(name);
}

bool FaseCore::linkNode(const std::string& src_node_name,
                        const size_t& src_arg_idx,
                        const std::string& dst_node_name,
                        const size_t& dst_arg_idx) {
    if (!exists(nodes, src_node_name) || exists(nodes, dst_node_name)) {
        return false;
    }
    if (nodes[dst_node_name].links.size() <= dst_arg_idx ||
        nodes[src_node_name].links.size() <= src_arg_idx) {
        return false;
    }

    // Check types
    if (nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
            nodes[src_node_name].arg_values[src_arg_idx])) {
        std::cerr << "Invalid types to create link" << std::endl;
        return false;
    }

    // Register
    nodes[dst_node_name].links[dst_arg_idx] = {src_node_name, src_arg_idx};
    return true;
};

bool FaseCore::setNodeArg(const std::string& node_name, const size_t arg_idx,
                          Variable arg) {
    if (!exists(nodes, node_name)) {
        return false;
    }
    if (nodes[node_name].links.size() <= arg_idx) {
        return false;
    }
    assert(nodes[node_name].arg_values.size() == nodes[node_name].links.size());

    // Check input type
    auto& arg_value = nodes[node_name].arg_values[arg_idx];
    if (!arg_value.isSameType(arg)) {
        std::cerr << "Invalid input type to set node argument" << std::endl;
        return false;
    }

    // Register
    arg_value = arg;
    return true;
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

    // Mark all node names as unused
    std::set<std::string> unused_node_names;
    extractKeys(nodes, unused_node_names);

    while (true) {
        // Find runnable node by checking link dependency
        const std::string& node_name =
                FindRunnableNode(unused_node_names, nodes, output_variables);
        if (node_name.empty()) {
            // No runnable node
            return true;
        }
        unused_node_names.erase(node_name);

        Node& node = nodes[node_name];
        size_t n_args = node.links.size();

        // Set output variable
        assert(output_variables[node_name].size() == 0);
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            if (link.node_name.empty()) {
                // Case 1: Create default argument
                const Variable& v = node.arg_values[arg_idx];
                output_variables[node_name].push_back(v.clone());
            } else {
                // Case 2: Use output variable
                Variable& v = output_variables.at(link.node_name)[link.arg_idx];
                output_variables[node_name].push_back(v);
            }
        }

        // Collect or create binding variables
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
    // TODO throw catch
    for (auto& f : pipeline) {
        f();
    }
    return true;
}

}  // namespace fase
