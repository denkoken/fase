#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

#include "core_util.h"

namespace fase {

namespace {

template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end(); it++) {
        dst_set.emplace(it->first);
    }
}

template <typename T>
T PopFront(std::vector<std::set<T>>& set_array) {
    if (set_array.empty()) {
        return T();
    }

    auto dst = *std::begin(set_array[0]);
    set_array[0].erase(dst);

    if (set_array[0].empty()) {
        set_array.erase(std::begin(set_array));
    }

    return dst;
}

}  // anonymous namespace

bool FaseCore::addNode(const std::string& name, const std::string& func_repr,
                       const int& phase) {
    if (name.empty()) {
        return false;
    }
    // check defined function name.
    if (!exists(functions, func_repr)) {
        return false;
    }

    // check uniqueness of name.
    if (exists(nodes, name)) {
        nodes[name].phase = phase;
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
                   .arg_values = arg_values,
                   .phase = phase};

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
    auto node_order = GetCallOrder(nodes);
    while (!PopFront(node_order).empty())
        ;
    if (!node_order.empty()) {
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
                          const std::string& arg_repr,
                          const Variable& arg_val) {
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

const std::map<std::string, Node>& FaseCore::getNodes() const {
    return nodes;
}

const std::map<std::string, Function>& FaseCore::getFunctions() const {
    return functions;
}

bool FaseCore::build() {
    pipeline.clear();
    output_variables.clear();

    // Stack for finding runnable node
    auto node_order = GetCallOrder(nodes);

    while (true) {
        // Find runnable node
        const std::string node_name = PopFront(node_order);
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
    assert(node_order.empty());

    return true;
}

bool FaseCore::run() {
    for (auto& f : pipeline) {
        f();
    }
    return true;
}

}  // namespace fase
