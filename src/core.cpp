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
inline bool exists(const std::vector<T>& vec, const T& key) {
    return std::find(std::begin(vec), std::end(vec), key) != std::end(vec);
}

template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end() ; it++) {
        dst_set.emplace(it->first);
    }
}

std::string FindRunnableNode(
        const std::set<std::string>& unused_node_names,
        const std::map<std::string, Function> &functions,
        const std::map<std::string, Node>& nodes,
        const std::map<std::string, std::vector<Variable>>& output_variables) {
    // Find runnable function node
    for (auto& node_name : unused_node_names) {
        const Node& node = nodes.at(node_name);
        const Function& func = functions.at(node.func_repr);

        bool runnable = true;
        size_t arg_idx = 0;
        for (auto& link: node.links) {
            if (link.node_name.empty()) {
                // Case 1: Create default argument
                // Is default argument given?
                if (func.default_arg_values.size() <= arg_idx) {
                    runnable = false;
                    break;
                }
            } else {
                // Case 2: Use output variable
                // Is linked node created?
                if (!exists(output_variables, link.node_name)) {
                    runnable = false;
                    break;
                }
                // Is argument index valid?
                const size_t n_args = output_variables.at(link.node_name).size();
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

    // Register node
    const size_t n_args = functions[func_repr].arg_reprs.size();
    nodes[name] = {.func_repr = func_repr,
                   .links = std::vector<Link>(n_args)};

    return true;
}

bool FaseCore::build() {
    pipeline.clear();
    output_variables.clear();

    // Mark all node names as unused
    std::set<std::string> unused_node_names;
    extractKeys(nodes, unused_node_names);

    while (true) {
        // Find runnable node by checking link dependency
        const std::string &node_name = FindRunnableNode(unused_node_names,
                                                        functions, nodes,
                                                        output_variables);
        if (node_name.empty()) {
            // No runnable node
            return true;
        }
        unused_node_names.erase(node_name);

        Node& node = nodes[node_name];
        Function& func = functions[node.func_repr];
        size_t n_args = node.links.size();

        // Set output variable
        assert(output_variables[node_name].size() == 0);
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            if (link.node_name.empty()) {
                // Case 1: Create default argument
                const Variable& v = func.default_arg_values[arg_idx];
                output_variables[node_name].push_back(v.clone());
            } else {
                // Case 2: Use output variable
                Variable &v = output_variables.at(link.node_name)[link.arg_idx];
                output_variables[node_name].push_back(v);
            }
        }

        // Collect or create binding variables
        std::vector<Variable*> bound_variables;
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            bound_variables.push_back(&output_variables[node_name][arg_idx]);
        }

        // TODO: Type check

        // Build
        pipeline.emplace_back(func.builder->build(bound_variables));
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
