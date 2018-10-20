#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <thread>

#include "core_util.h"

namespace fase {

namespace {

void del(const Link& l, std::vector<std::tuple<size_t, Link>>* rev_links) {
    for (auto i = std::begin(*rev_links); i != std::end(*rev_links); i++) {
        auto& r_l = std::get<1>(*i);
        if (r_l.node_name == l.node_name && r_l.arg_idx == l.arg_idx) {
            rev_links->erase(i);
            return;
        }
    }
}

void delRevLink(const Node& node, const size_t& idx, FaseCore* core) {
    while (true) {
        bool f = true;
        for (auto& pair : node.rev_links) {
            auto link = std::get<1>(pair);

            if (std::get<0>(pair) != idx) {
                continue;
            };
            core->delLink(link.node_name, link.arg_idx);
            f = false;
            break;
        }
        if (f) break;
    }
}

bool checkVarName(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    std::string invalid_strs[] = {
            " ",  "__", ",", "@", ":", "&", "%", "+", "-",
            "\\", "^",  "~", "=", "(", ")", "#", "$", "\"",
            "!",  "<",  ">", "?", "{", "}", "[", "]", "`",
    };

    for (auto& str : invalid_strs) {
        if (name.find(str) != std::string::npos) {
            return false;
        }
    }

    // check _ + Large character start
    for (size_t i = 0; i < 27; i++) {
        if (name.find("_" + std::string({char('A' + i)})) == 0) {
            return false;
        }
    }
    // check Number start
    for (size_t i = 0; i < 10; i++) {
        if (name.find(std::string({char('0' + i)})) == 0) {
            return false;
        }
    }

    return true;
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

void initInOutFunc(const std::string& project, FaseCore* core) {
    std::function<void()> dummy = [] {};
    core->addFunctionBuilder(InputFuncStr(project), dummy, {}, {}, {}, {});
    core->addFunctionBuilder(OutputFuncStr(project), dummy, {}, {}, {}, {});
}

void initNode(const std::string& project, std::map<std::string, Node>* nodes) {
    (*nodes)[InputNodeStr()] = {InputFuncStr(project),
                                std::vector<Link>(),
                                std::vector<std::tuple<size_t, Link>>(),
                                std::vector<std::string>(),
                                std::vector<Variable>(),
                                std::numeric_limits<int>::min()};

    (*nodes)[OutputNodeStr()] = {OutputFuncStr(project),
                                 std::vector<Link>(),
                                 std::vector<std::tuple<size_t, Link>>(),
                                 std::vector<std::string>(),
                                 std::vector<Variable>(),
                                 std::numeric_limits<int>::max()};
}

std::function<void()> wrapPipe(const std::string& node_name,
                               std::function<void()>&& f) {
    return [node_name, f = std::forward<std::function<void()>>(f)]() {
        try {
            f();
        } catch (std::exception& e) {
            throw ErrorThrownByNode(node_name, e.what());
        } catch (std::string e) {
            throw ErrorThrownByNode(node_name, e);
        } catch (long long e) {
            throw ErrorThrownByNode(node_name, std::to_string(e));
        } catch (...) {
            throw ErrorThrownByNode(node_name, "something went wrong.");
        }
    };
}

}  // anonymous namespace

bool FaseCore::checkNodeName(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    if (IsSpecialNodeName(name)) {
        return false;
    }

    // check uniqueness of name.
    if (exists(projects[primary_project].nodes, name)) {
        return false;
    }

    return true;
}

FaseCore::FaseCore() {
    primary_project = "Untitled";
    projects[primary_project] = {};

    initInOutFunc(primary_project, this);
    // make input node and output node.
    initNode(primary_project, &projects[primary_project].nodes);
}

bool FaseCore::addNode(const std::string& name, const std::string& func_repr,
                       const int& priority) {
    // check uniqueness of name and...
    if (!checkNodeName(name)) {
        return false;
    }

    // check defined function name.
    if (!exists(functions, func_repr)) {
        return false;
    }

    // Make clone of default variables
    std::vector<Variable> arg_values;
    for (auto& v : functions[func_repr].default_arg_values) {
        arg_values.push_back(v.clone());
    }

    // Register node (arg_values are copied from function's default_arg_values)
    const size_t n_args = functions[func_repr].arg_type_reprs.size();
    projects[primary_project].nodes[name] = {
            func_repr,
            std::vector<Link>(n_args),
            std::vector<std::tuple<size_t, Link>>(),
            functions[func_repr].default_arg_reprs,
            arg_values,
            priority};

    return true;
}

bool FaseCore::delNode(const std::string& node_name) noexcept {
    auto& nodes = projects[primary_project].nodes;
    if (!exists(nodes, node_name) || IsSpecialNodeName(node_name)) {
        return false;
    }
    // Remove connected links
    for (auto& r_l : nodes[node_name].rev_links) {
        if (!exists(nodes, std::get<1>(r_l).node_name)) {
            std::cerr << "err" << std::endl;
            std::cerr << std::get<1>(r_l).node_name << std::endl;
            continue;
        }
        nodes[std::get<1>(r_l).node_name].links[std::get<1>(r_l).arg_idx] = {};
    }
    // Remove connected reverse links
    for (size_t i = 0; i < nodes[node_name].links.size(); i++) {
        auto& link = nodes[node_name].links[i];
        if (link.node_name.empty()) {
            continue;
        }
        del({node_name, i}, &nodes[link.node_name].rev_links);
    }

    // Remove node
    nodes.erase(node_name);

    // std::cout << "///////" << std::endl;
    // // for debug
    // for (auto& pair : nodes) {
    //     auto& rev_links = std::get<1>(pair).rev_links;
    //
    //     std::cout << std::get<0>(pair) << std::endl;
    //     for (auto& tup : rev_links) {
    //         std::cout << "    " << std::get<0>(tup) << " : "
    //                   << std::get<1>(tup).node_name << "  "
    //                   << std::get<1>(tup).arg_idx << std::endl;
    //     }
    // }
    return true;
}

bool FaseCore::renameNode(const std::string& old_name,
                          const std::string& new_name) {
    auto& nodes = projects[primary_project].nodes;
    if (!exists(nodes, old_name) || !checkNodeName(new_name) ||
        IsSpecialNodeName(old_name)) {
        return false;
    }

    Node buf = nodes[old_name];
    delNode(old_name);
    nodes[new_name] = buf;
    // Links
    for (size_t i = 0; i < buf.links.size(); i++) {
        addLink(buf.links[i].node_name, buf.links[i].arg_idx, new_name, i);
    }
    // Reverce Links
    nodes[new_name].rev_links.clear();
    for (auto& r_l : buf.rev_links) {
        addLink(new_name, std::get<0>(r_l), std::get<1>(r_l).node_name,
                std::get<1>(r_l).arg_idx);
    }

    return true;
}

bool FaseCore::addLink(const std::string& src_node_name,
                       const size_t& src_arg_idx,
                       const std::string& dst_node_name,
                       const size_t& dst_arg_idx) {
    auto& nodes = projects[primary_project].nodes;
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

    // Check link existence
    if (!nodes[dst_node_name].links[dst_arg_idx].node_name.empty()) {
        return false;
    }
    if (dst_node_name == InputNodeStr() && src_node_name == OutputNodeStr()) {
        // TODO add recursive link
        return false;
    }
    if (dst_node_name == InputNodeStr()) {
        return false;
    } else if (src_node_name == OutputNodeStr()) {
        return false;
    }

    if (src_node_name == InputNodeStr() &&
        !projects[primary_project].is_locked_inout &&
        !nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                nodes[src_node_name].arg_values[src_arg_idx])) {
        const Function& dst_func = functions[nodes[dst_node_name].func_repr];
        Function& func = functions[InputFuncStr(primary_project)];

        func.arg_type_reprs[src_arg_idx] = dst_func.arg_type_reprs[dst_arg_idx];
        func.arg_types[src_arg_idx] = dst_func.arg_types[dst_arg_idx];
        func.default_arg_reprs[src_arg_idx] =
                dst_func.default_arg_reprs[dst_arg_idx];

        func.default_arg_values[src_arg_idx] =
                dst_func.default_arg_values[dst_arg_idx].clone();

        nodes[InputNodeStr()].arg_values[src_arg_idx] =
                nodes[dst_node_name].arg_values[dst_arg_idx].clone();

        delRevLink(nodes[InputNodeStr()], src_arg_idx, this);
    } else if (dst_node_name == OutputNodeStr() &&
               !projects[primary_project].is_locked_inout &&
               !nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                       nodes[src_node_name].arg_values[src_arg_idx])) {
        const Function& src_func = functions[nodes[src_node_name].func_repr];
        Function& func = functions[OutputFuncStr(primary_project)];

        func.arg_type_reprs[dst_arg_idx] = src_func.arg_type_reprs[src_arg_idx];
        func.arg_types[dst_arg_idx] = src_func.arg_types[src_arg_idx];
        func.default_arg_reprs[dst_arg_idx] =
                src_func.default_arg_reprs[src_arg_idx];

        func.default_arg_values[dst_arg_idx] =
                src_func.default_arg_values[src_arg_idx].clone();

        nodes[OutputNodeStr()].arg_values[dst_arg_idx] =
                nodes[src_node_name].arg_values[src_arg_idx].clone();
    }

    // Check types
    if (!nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                nodes[src_node_name].arg_values[src_arg_idx])) {
        std::cerr << "Invalid types to create link" << std::endl;
        return false;
    }

    // Register
    nodes[dst_node_name].links[dst_arg_idx] = {src_node_name, src_arg_idx};
    nodes[src_node_name].rev_links.push_back(
            {src_arg_idx, Link{dst_node_name, dst_arg_idx}});

    // Test for loop link
    auto node_order = GetCallOrder(nodes);
    if (node_order.empty()) {
        // Revert registration
        nodes[dst_node_name].links[dst_arg_idx] = {};
        nodes[src_node_name].rev_links.pop_back();
        return false;
    }

    return true;
}

void FaseCore::delLink(const std::string& dst_node_name,
                       const size_t& dst_arg_idx) {
    auto& nodes = projects[primary_project].nodes;
    if (!exists(nodes, dst_node_name)) {
        return;
    }
    if (nodes[dst_node_name].links.size() <= dst_arg_idx) {
        return;
    }
    if (nodes[dst_node_name].links[dst_arg_idx].node_name.empty()) {
        return;
    }

    // Delete
    Link l = nodes[dst_node_name].links[dst_arg_idx];
    auto& r_ls = nodes[l.node_name].rev_links;
    for (auto iter = std::begin(r_ls); iter != std::end(r_ls); iter++) {
        if (std::get<1>(*iter).arg_idx == dst_arg_idx &&
            std::get<1>(*iter).node_name == dst_node_name) {
            r_ls.erase(iter);
            break;
        }
    }
    nodes[dst_node_name].links[dst_arg_idx] = {};
}

bool FaseCore::setPriority(const std::string& node_name, const int& priority) {
    auto& nodes = projects[primary_project].nodes;
    if (!exists(nodes, node_name) || IsSpecialNodeName(node_name)) {
        return false;
    }
    nodes[node_name].priority = priority;
    return true;
}

bool FaseCore::setNodeArg(const std::string& node_name, const size_t arg_idx,
                          const std::string& arg_repr,
                          const Variable& arg_val) {
    auto& nodes = projects[primary_project].nodes;
    if (!exists(nodes, node_name)) {
        return false;
    }
    Node& node = nodes[node_name];
    if (node.links.size() <= arg_idx) {
        return false;
    }

    // Check input type (through if arg_value is empty.)
    if (node.arg_values[arg_idx] &&
        !node.arg_values[arg_idx].isSameType(arg_val)) {
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
    auto& nodes = projects[primary_project].nodes;
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

bool FaseCore::addInput(const std::string& name, const std::type_info* type) {
    if (!checkVarName(name)) {
        return false;
    }

    if (exists(functions[InputFuncStr(primary_project)].arg_names, name)) {
        return false;
    }

    if (projects[primary_project].is_locked_inout) {
        return false;
    }

    Function& func = functions[InputFuncStr(primary_project)];
    func.arg_type_reprs.push_back("");
    func.arg_types.push_back(type);
    func.default_arg_reprs.push_back("");
    func.arg_names.push_back(name);
    func.default_arg_values.push_back(Variable());
    func.is_input_args.push_back(false);

    Node& node = projects[primary_project].nodes[InputNodeStr()];

    node.arg_reprs.push_back(name);
    node.links.push_back({});
    node.arg_values.push_back(Variable());

    return true;
}

bool FaseCore::delInput(const size_t& idx) {
    if (idx >= functions[InputFuncStr(primary_project)].arg_names.size()) {
        return false;
    }

    if (projects[primary_project].is_locked_inout) {
        return false;
    }

    Node& node = projects[primary_project].nodes[InputNodeStr()];
    Function& func = functions[InputFuncStr(primary_project)];

    // store all linking info, and delete all links.
    const std::vector<std::tuple<size_t, Link>> rev_links_buf = node.rev_links;
    for (size_t i = 0; i < node.links.size(); i++) {
        delRevLink(node, i, this);
    }

    func.arg_type_reprs.erase(std::begin(func.arg_type_reprs) + long(idx));
    func.arg_types.erase(std::begin(func.arg_types) + long(idx));
    func.default_arg_reprs.erase(std::begin(func.default_arg_reprs) +
                                 long(idx));
    func.arg_names.erase(std::begin(func.arg_names) + long(idx));
    func.default_arg_values.erase(std::begin(func.default_arg_values) +
                                  long(idx));
    func.is_input_args.erase(std::begin(func.is_input_args) + long(idx));

    node.arg_reprs.erase(std::begin(node.arg_reprs) + long(idx));
    node.links.erase(std::begin(node.links) + long(idx));
    node.arg_values.erase(std::begin(node.arg_values) + long(idx));

    for (const auto& old_rev_link : rev_links_buf) {
        size_t link_slot = std::get<0>(old_rev_link);
        if (link_slot == idx) {
            continue;
        } else if (link_slot > idx) {
            link_slot -= 1;
        }
        Link link = std::get<1>(old_rev_link);
        addLink(InputNodeStr(), link_slot, link.node_name, link.arg_idx);
    }

    return true;
}

bool FaseCore::addOutput(const std::string& name, const std::type_info* type) {
    if (!checkVarName(name)) {
        return false;
    }

    if (exists(functions[OutputFuncStr(primary_project)].arg_names, name)) {
        return false;
    }

    if (projects[primary_project].is_locked_inout) {
        return false;
    }

    Function& func = functions[OutputFuncStr(primary_project)];
    func.arg_type_reprs.push_back("");
    func.arg_types.push_back(type);
    func.default_arg_reprs.push_back("");
    func.arg_names.push_back(name);
    func.default_arg_values.push_back(Variable());
    func.is_input_args.push_back(true);

    Node& node = projects[primary_project].nodes[OutputNodeStr()];

    node.arg_reprs.push_back(name);
    node.links.push_back({});
    node.arg_values.push_back(Variable());

    return true;
}

bool FaseCore::delOutput(const size_t& idx) {
    if (idx >= functions[OutputFuncStr(primary_project)].arg_names.size()) {
        return false;
    }

    if (projects[primary_project].is_locked_inout) {
        return false;
    }

    Node& node = projects[primary_project].nodes[OutputNodeStr()];
    Function& func = functions[OutputFuncStr(primary_project)];

    // store all linking info, and delete all links.
    const std::vector<Link> links_buf = node.links;
    for (size_t i = 0; i < node.links.size(); i++) {
        delLink(OutputNodeStr(), i);
    }

    func.arg_type_reprs.erase(std::begin(func.arg_type_reprs) + long(idx));
    func.arg_types.erase(std::begin(func.arg_types) + long(idx));
    func.default_arg_reprs.erase(std::begin(func.default_arg_reprs) +
                                 long(idx));
    func.arg_names.erase(std::begin(func.arg_names) + long(idx));
    func.default_arg_values.erase(std::begin(func.default_arg_values) +
                                  long(idx));
    func.is_input_args.erase(std::begin(func.is_input_args) + long(idx));

    node.arg_reprs.erase(std::begin(node.arg_reprs) + long(idx));
    node.links.erase(std::begin(node.links) + long(idx));
    node.arg_values.erase(std::begin(node.arg_values) + long(idx));

    for (size_t i = 0; i < node.links.size(); i++) {
        size_t link_slot = i;
        if (i == idx) {
            continue;
        } else if (i > idx) {
            link_slot -= 1;
        }
        const auto& link = links_buf[link_slot];
        addLink(link.node_name, link.arg_idx, OutputNodeStr(), link_slot);
    }

    return true;
}

void FaseCore::lockInOut() {
    projects[primary_project].is_locked_inout = true;
}
void FaseCore::unlockInOut() {
    projects[primary_project].is_locked_inout = false;
}

void FaseCore::switchPipeline(const std::string& project_name) noexcept {
    primary_project = project_name;

    if (projects[primary_project].nodes.empty()) {
        initInOutFunc(primary_project, this);
        initNode(primary_project, &projects[primary_project].nodes);
    }
}

void FaseCore::renamePipeline(const std::string& project_name) noexcept {
    Pipeline buf = std::move(projects[primary_project]);
    projects.erase(primary_project);
    primary_project = project_name;
    projects[primary_project] = std::move(buf);
}

void FaseCore::deletePipeline(const std::string& project_name) noexcept {
    if (primary_project == project_name) {
        return;
    }
    projects.erase(project_name);
}

const std::string& FaseCore::getCurrentPipelineName() const noexcept {
    return primary_project;
}

const std::map<std::string, Node>& FaseCore::getNodes() const {
    return projects.at(primary_project).nodes;
}

const std::map<std::string, Function>& FaseCore::getFunctions() const {
    return functions;
}

std::vector<std::string> FaseCore::getPipelineNames() const {
    std::vector<std::string> dst;
    for (const auto& pair : projects) {
        dst.emplace_back(std::get<0>(pair));
    }
    return dst;
}

const std::vector<Variable>& FaseCore::getOutputs() const {
    return output_variables.at(OutputNodeStr());
}

std::function<void()> FaseCore::buildNode(
        const std::string& node_name, const std::vector<Variable*>& args,
        std::map<std::string, ResultReport>* report_box_) const {
    const Function& func = functions.at(
            projects.at(primary_project).nodes.at(node_name).func_repr);
    if (node_name == InputNodeStr() || node_name == OutputNodeStr()) {
        return [] {};
    }
    if (report_box_ != nullptr) {
        return wrapPipe(node_name,
                        func.builder->build(args, &(*report_box_)[node_name]));
    } else {
        return wrapPipe(node_name, func.builder->build(args));
    }
}

void FaseCore::buildNodesParallel(
        const std::set<std::string>& runnables,
        const size_t& step,  // for report.
        std::map<std::string, ResultReport>* report_box_) {
    auto& nodes = projects[primary_project].nodes;
    std::map<std::string, std::vector<Variable*>> variable_ps;
    for (auto& runnable : runnables) {
        variable_ps[runnable] = BindVariables(nodes[runnable], output_variables,
                                              &output_variables[runnable]);
    }

    // Build
    std::vector<std::function<void()>> funcs;
    for (auto& runnable : runnables) {
        funcs.emplace_back(
                buildNode(runnable, variable_ps[runnable], report_box_));
    }
    if (report_box_ != nullptr) {
        built_pipeline.push_back([funcs, report_box_, step] {
            auto start = std::chrono::system_clock::now();
            std::vector<std::thread> ths;
            std::exception_ptr ep;
            for (auto& func : funcs) {
                ths.emplace_back([&ep, &func]() {
                    try {
                        func();
                    } catch (...) {
                        ep = std::current_exception();
                    }
                });
            }
            for (auto& th : ths) {
                th.join();
            }
            if (ep) {
                std::rethrow_exception(ep);
            }
            (*report_box_)[StepStr(step)].execution_time =
                    std::chrono::system_clock::now() - start;
        });
    } else {
        built_pipeline.push_back([funcs] {
            std::vector<std::thread> ths;
            std::exception_ptr ep;
            for (auto& func : funcs) {
                ths.emplace_back([&ep, &func]() {
                    try {
                        func();
                    } catch (...) {
                        ep = std::current_exception();
                    }
                });
            }
            for (auto& th : ths) {
                th.join();
            }
            if (ep) {
                std::rethrow_exception(ep);
            }
        });
    }
}

void FaseCore::buildNodesNonParallel(
        const std::set<std::string>& runnables,
        std::map<std::string, ResultReport>* report_box_) {
    auto& nodes = projects[primary_project].nodes;
    for (auto& runnable : runnables) {
        const Node& node = nodes[runnable];
        auto bound_variables = BindVariables(node, output_variables,
                                             &output_variables[runnable]);

        // Build
        built_pipeline.emplace_back(
                buildNode(runnable, bound_variables, report_box_));
    }
}

bool FaseCore::build(bool parallel_exe, bool profile) {
    auto& nodes = projects[primary_project].nodes;
    projects[primary_project].multi = parallel_exe;
    built_pipeline.clear();
    output_variables.clear();
    report_box.clear();

    // TODO input/output

    // Build running order.
    auto node_order = GetCallOrder(nodes);
    assert(!node_order.empty());

    if (profile) {
        auto start = std::make_shared<std::chrono::system_clock::time_point>();
        built_pipeline.push_back(
                [start] { *start = std::chrono::system_clock::now(); });

        size_t step = 0;
        for (auto& runnables : node_order) {
            if (parallel_exe) {
                buildNodesParallel(runnables, step++, &report_box);
            } else {
                buildNodesNonParallel(runnables, &report_box);
            }
        }

        built_pipeline.push_back([start, &report_box = this->report_box] {
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
    for (auto& f : built_pipeline) {
        f();
    }
    return report_box;
}

}  // namespace fase
