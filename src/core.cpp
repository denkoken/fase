#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <thread>

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

std::vector<Variable*> BindVariables(
        const Node& node,
        const std::map<std::string, std::vector<Variable>>& exists_variables,
        std::vector<Variable>* variables) {
    assert(variables->empty());

    const size_t n_args = node.links.size();
    // Set output variable
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        const auto& link = node.links[arg_idx];
        if (link.node_name.empty()) {
            // Case 1: Create default argument
            const Variable& v = node.arg_values[arg_idx];
            variables->push_back(v);
        } else {
            // Case 2: Use output variable
            variables->push_back(
                    exists_variables.at(link.node_name)[link.arg_idx]);
        }
    }

    // Collect binding variables
    std::vector<Variable*> bound_variables;
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        bound_variables.push_back(&variables->at(arg_idx));
    }
    return bound_variables;
}

}  // anonymous namespace

FaseCore::FaseCore() {
    // TODO
    // nodes[ReportHeaderStr() + std::string("__input")] = {
    //     .func_repr="",
    //     .links = std::vector<Link>(),
    //     .arg_reprs = std::vector<std::string>(),
    //     .arg_values = std::vector<Variable>(),
    //     .phase = INT_MIN
    // };
}

bool FaseCore::addNode(const std::string& name, const std::string& func_repr,
                       const int& priority) {
    if (name.empty()) {
        return false;
    }

    if (name.find(ReportHeaderStr()) != std::string::npos) {
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
                   .rev_links = std::vector<std::vector<Link>>(n_args),
                   .arg_reprs = functions[func_repr].default_arg_reprs,
                   .arg_values = arg_values,
                   .priority = priority};

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
    nodes[src_node_name].rev_links[src_arg_idx].push_back(
            {dst_node_name, dst_arg_idx});

    // Test for loop link
    auto node_order = GetCallOrder(nodes);
    if (node_order.empty()) {
        // Revert registration
        nodes[dst_node_name].links[dst_arg_idx] = {};
        nodes[src_node_name].rev_links[src_arg_idx].pop_back();
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
    Link l = nodes[dst_node_name].links[dst_arg_idx];
    std::vector<Link>& rev_links = nodes[l.node_name].rev_links[l.arg_idx];
    for (auto r_l = std::begin(rev_links); r_l != std::end(rev_links); r_l++) {
        if (r_l->node_name == dst_node_name && r_l->arg_idx == dst_arg_idx) {
            rev_links.erase(r_l);
            break;
        }
    }
    nodes[dst_node_name].links[dst_arg_idx] = {};
}

bool FaseCore::setPriority(const std::string& node_name, const int& priority) {
    if (!exists(nodes, node_name)) {
        return false;
    }
    nodes[node_name].priority = priority;
    return true;
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

std::function<void()> FaseCore::buildNode(
        const std::string& node_name, const std::vector<Variable*>& args,
        std::map<std::string, ResultReport>* report_box) const {
    const Function& func = functions.at(nodes.at(node_name).func_repr);
    if (report_box != nullptr) {
        return func.builder->build(args, &(*report_box)[node_name]);
    } else {
        return func.builder->build(args);
    }
}

void FaseCore::buildNodesParallel(
        const std::set<std::string>& runnables,
        const size_t& step,  // for report.
        std::map<std::string, ResultReport>* report_box) {
    std::map<std::string, std::vector<Variable*>> variable_ps;
    for (auto& runnable : runnables) {
        variable_ps[runnable] = BindVariables(nodes[runnable], output_variables,
                                              &output_variables[runnable]);
    }

    // Build
    std::vector<std::function<void()>> funcs;
    for (auto& runnable : runnables) {
        funcs.emplace_back(
                buildNode(runnable, variable_ps[runnable], report_box));
    }
    if (report_box != nullptr) {
        pipeline.push_back([funcs, report_box, step] {
            auto start = std::chrono::system_clock::now();
            std::vector<std::thread> ths;
            for (auto& func : funcs) {
                ths.emplace_back(func);
            }
            for (auto& th : ths) {
                th.join();
            }
            (*report_box)[StepStr(step)].execution_time =
                    std::chrono::system_clock::now() - start;
        });
    } else {
        pipeline.push_back([funcs] {
            std::vector<std::thread> ths;
            for (auto& func : funcs) {
                ths.emplace_back(func);
            }
            for (auto& th : ths) {
                th.join();
            }
        });
    }
}

void FaseCore::buildNodesNonParallel(
        const std::set<std::string>& runnables,
        std::map<std::string, ResultReport>* report_box) {
    for (auto& runnable : runnables) {
        const Node& node = nodes[runnable];
        auto bound_variables = BindVariables(node, output_variables,
                                             &output_variables[runnable]);

        // Build
        pipeline.emplace_back(buildNode(runnable, bound_variables, report_box));
    }
}

bool FaseCore::build(bool parallel_exe, bool profile) {
    pipeline.clear();
    output_variables.clear();
    report_box.clear();

    // Build running order.
    auto node_order = GetCallOrder(nodes);
    assert(!node_order.empty());

    if (profile) {
        auto start = std::make_shared<std::chrono::system_clock::time_point>();
        pipeline.push_back(
                [start] { *start = std::chrono::system_clock::now(); });

        size_t step = 0;
        for (auto& runnables : node_order) {
            if (parallel_exe) {
                buildNodesParallel(runnables, step++, &report_box);
            } else {
                buildNodesNonParallel(runnables, &report_box);
            }
        }

        pipeline.push_back([start, &report_box = this->report_box] {
            report_box[TotalTimeStr()].execution_time =
                    std::chrono::system_clock::now() - *start;
        });
    } else {
        for (auto& runnables : node_order) {
            if (parallel_exe) {
                buildNodesParallel(runnables, 0, nullptr);
            } else {
                buildNodesNonParallel(runnables, nullptr);
            }
        }
    }

    return true;
}

const std::map<std::string, ResultReport>& FaseCore::run() {
    for (auto& f : pipeline) {
        f();
    }
    return report_box;
}

}  // namespace fase
