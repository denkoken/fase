#include "core.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

#include "core_util.h"

#include "debug_macros.h"

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

template <class Conrainer>
void Erase(Conrainer& cont, size_t idx) {
    cont.erase(std::begin(cont) + long(idx));
}

template <typename Key, typename T>
std::vector<Key> getKeys(const std::map<Key, T>& map) {
    std::vector<Key> dst;
    for (const auto& pair : map) {
        dst.emplace_back(std::get<0>(pair));
    }
    return dst;
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

template <class Conrainer>
size_t getIndex(const Conrainer& cont,
                const typename Conrainer::value_type& v) {
    return size_t(std::find(std::begin(cont), std::end(cont), v) -
                  std::begin(cont));
}

void genNode(const std::string& func_repr, const std::string& node_name,
             const int& priority, std::map<std::string, Function>* funcs,
             std::map<std::string, Node>* nodes) {
    // Make clone of default variables
    std::vector<Variable> arg_values;
    for (auto& v : funcs->at(func_repr).default_arg_values) {
        arg_values.push_back(v.clone());
    }

    // Register node (arg_values are copied from function's
    // default_arg_values)
    const size_t n_args = funcs->at(func_repr).is_input_args.size();
    // clang-format off
    (*nodes)[node_name] = {
        func_repr,
        std::vector<Link>(n_args),
        std::vector<std::tuple<size_t, Link>>(),
        funcs->at(func_repr).default_arg_reprs,
        arg_values,
        priority
    };
    // clang-format on
}

}  // anonymous namespace

Pipeline& FaseCore::getCurrentPipeline() {
    if (pipelines.count(current_pipeline)) {
        return pipelines[current_pipeline];
    } else if (sub_pipelines.count(current_pipeline)) {
        return sub_pipelines[current_pipeline];
    } else {
        return pipelines[current_pipeline];
    }
}

const Pipeline& FaseCore::getCurrentPipeline() const {
    if (pipelines.count(current_pipeline)) {
        return pipelines.at(current_pipeline);
    } else if (sub_pipelines.count(current_pipeline)) {
        return sub_pipelines.at(current_pipeline);
    } else {
        return pipelines.at(current_pipeline);
    }
}

bool FaseCore::checkNodeName(const std::string& name) {
    if (name.empty()) {
        return false;
    }

    if (IsSpecialNodeName(name)) {
        return false;
    }

    // check uniqueness of name.
    if (exists(getCurrentPipeline().nodes, name)) {
        return false;
    }

    return true;
}

const char FaseCore::MainPipeInOutName[] = "main__";

std::string FaseCore::getEdittingInputFunc() {
    if (pipelines.count(current_pipeline)) {
        return InputFuncStr(MainPipeInOutName);
    }
    return InputFuncStr(current_pipeline);
}
std::string FaseCore::getEdittingOutputFunc() {
    if (pipelines.count(current_pipeline)) {
        return OutputFuncStr(MainPipeInOutName);
    }
    return OutputFuncStr(current_pipeline);
}

void FaseCore::initInOut() {
    // if there is not input/output function builder, make theirs.
    if ((!functions.count(getEdittingInputFunc())) &&
        (!functions.count(getEdittingOutputFunc()))) {
        std::function<void()> dummy = [] {};

        addFunctionBuilder(getEdittingInputFunc(), dummy, {}, {}, {});
        addFunctionBuilder(getEdittingOutputFunc(), dummy, {}, {}, {});
    }

    // make input node
    genNode(getEdittingInputFunc(), InputNodeStr(),
            std::numeric_limits<int>::min(), &functions,
            &getCurrentPipeline().nodes);

    // make output node
    genNode(getEdittingOutputFunc(), OutputNodeStr(),
            std::numeric_limits<int>::max(), &functions,
            &getCurrentPipeline().nodes);
}

FaseCore::FaseCore(FaseCore& another)
    : functions(another.functions),
      pipelines(another.pipelines),
      current_pipeline(another.current_pipeline),
      main_pipeline_last_selected(another.main_pipeline_last_selected),
      sub_pipelines(another.sub_pipelines),
      sub_pipeline_fbs(another.sub_pipeline_fbs),
      version(another.version),
      is_locked_inout(another.is_locked_inout),
      built_pipeline(),
      output_variables(),
      built_version(0),
      is_profiling_built(another.is_profiling_built),
      report_box() {}

FaseCore& FaseCore::operator=(FaseCore& another) {
    functions = another.functions;
    pipelines = another.pipelines;
    sub_pipelines = another.sub_pipelines;
    sub_pipeline_fbs = another.sub_pipeline_fbs;
    current_pipeline = another.current_pipeline;
    main_pipeline_last_selected = another.main_pipeline_last_selected;
    version = another.version;
    built_pipeline = decltype(built_pipeline)();

    is_locked_inout = another.is_locked_inout;

    output_variables = decltype(output_variables)();
    built_version = 0;
    is_profiling_built = another.is_profiling_built;
    report_box = decltype(report_box)();
    return *this;
}

FaseCore::FaseCore() : version(0) {
    current_pipeline = "Untitled";
    main_pipeline_last_selected = current_pipeline;
    getCurrentPipeline() = {};

    // make input/output node and function builder.
    initInOut();
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

    // recursive nest node is NG.
    if (SubPipelineFuncStr(current_pipeline) == func_repr) {
        return false;
    }

    genNode(func_repr, name, priority, &functions, &getCurrentPipeline().nodes);

    version++;

    return true;
}

bool FaseCore::delNode(const std::string& node_name) noexcept {
    auto& nodes = getCurrentPipeline().nodes;
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
    version++;
    return true;
}

bool FaseCore::renameNode(const std::string& old_name,
                          const std::string& new_name) {
    auto& nodes = getCurrentPipeline().nodes;
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
    auto& nodes = getCurrentPipeline().nodes;
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
        (!is_locked_inout || sub_pipelines.count(current_pipeline)) &&
        !nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                nodes[src_node_name].arg_values[src_arg_idx])) {
        const Function& dst_func = functions[nodes[dst_node_name].func_repr];
        Function& func = functions[getEdittingInputFunc()];

#define Copy(memb) func.memb[src_arg_idx] = dst_func.memb[dst_arg_idx]
        Copy(arg_types);
        Copy(default_arg_reprs);
        Copy(default_arg_values);
#undef Copy

        nodes[InputNodeStr()].arg_values[src_arg_idx] =
                nodes[dst_node_name].arg_values[dst_arg_idx].clone();

        delRevLink(nodes[InputNodeStr()], src_arg_idx, this);

        // for current pipeline is sub pipeline.
        if (functions.count(SubPipelineFuncStr(current_pipeline))) {
            auto f_name = SubPipelineFuncStr(current_pipeline);
            Function& sub_f = functions[f_name];

#define Copy(memb) sub_f.memb[src_arg_idx] = dst_func.memb[dst_arg_idx]
            Copy(arg_types);
            Copy(default_arg_reprs);
            Copy(default_arg_values);
#undef Copy

            auto f = [&](Pipeline& pipe) {
                for (auto& pair : pipe.nodes) {
                    if (std::get<1>(pair).func_repr == f_name) {
                        Node& o_node = std::get<1>(pair);
                        o_node.arg_values[src_arg_idx] =
                                dst_func.default_arg_values[dst_arg_idx];
                        o_node.arg_reprs[src_arg_idx] =
                                dst_func.default_arg_reprs[dst_arg_idx];
                    }
                }
            };

            for (auto& pipeline_p : pipelines) {
                f(std::get<1>(pipeline_p));
            }

            for (auto& pipeline_p : sub_pipelines) {
                f(std::get<1>(pipeline_p));
            }
        }
    } else if (dst_node_name == OutputNodeStr() &&
               (!is_locked_inout || sub_pipelines.count(current_pipeline)) &&
               !nodes[dst_node_name].arg_values[dst_arg_idx].isSameType(
                       nodes[src_node_name].arg_values[src_arg_idx])) {
        const Function& src_func = functions[nodes[src_node_name].func_repr];
        Function& func = functions[getEdittingOutputFunc()];

#define Copy(memb) func.memb[dst_arg_idx] = src_func.memb[src_arg_idx]
        Copy(arg_types);
        Copy(default_arg_reprs);
        Copy(default_arg_values);
#undef Copy

        nodes[OutputNodeStr()].arg_values[dst_arg_idx] =
                nodes[src_node_name].arg_values[src_arg_idx].clone();

        // for current pipeline is sub pipeline.
        if (functions.count(SubPipelineFuncStr(current_pipeline))) {
            auto f_name = SubPipelineFuncStr(current_pipeline);
            Function& sub_f = functions[f_name];

            // find output start index, and compute the copying index from it.
            size_t idx = getIndex(sub_f.is_input_args, false) + dst_arg_idx;

#define Copy(memb) sub_f.memb[idx] = src_func.memb[src_arg_idx]
            Copy(arg_types);
            Copy(default_arg_reprs);
            Copy(default_arg_values);
#undef Copy

            auto f = [&](Pipeline& pipe) {
                for (auto& pair : pipe.nodes) {
                    if (std::get<1>(pair).func_repr == f_name) {
                        Node& o_node = std::get<1>(pair);
                        o_node.arg_values[idx] =
                                src_func.default_arg_values[src_arg_idx];
                        o_node.arg_reprs[idx] =
                                src_func.default_arg_reprs[src_arg_idx];
                    }
                }
            };

            for (auto& pipeline_p : pipelines) {
                f(std::get<1>(pipeline_p));
            }

            for (auto& pipeline_p : sub_pipelines) {
                f(std::get<1>(pipeline_p));
            }
        }
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

    version++;

    return true;
}

void FaseCore::delLink(const std::string& dst_node_name,
                       const size_t& dst_arg_idx) {
    auto& nodes = getCurrentPipeline().nodes;
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

    version++;
}

bool FaseCore::setPriority(const std::string& node_name, const int& priority) {
    auto& nodes = getCurrentPipeline().nodes;
    if (!exists(nodes, node_name) || IsSpecialNodeName(node_name)) {
        return false;
    }
    nodes[node_name].priority = priority;
    version++;
    return true;
}

bool FaseCore::setNodeArg(const std::string& node_name, const size_t arg_idx,
                          const std::string& arg_repr,
                          const Variable& arg_val) {
    auto& nodes = getCurrentPipeline().nodes;
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

    version++;
    return true;
}

void FaseCore::clearNodeArg(const std::string& node_name,
                            const size_t arg_idx) {
    auto& nodes = getCurrentPipeline().nodes;
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

    version++;
}

bool FaseCore::addInput(const std::string& name, const std::type_info* type,
                        const Variable& default_value,
                        const std::string& default_arg_repr) {
    if (is_locked_inout && !sub_pipelines.count(current_pipeline)) {
        return false;
    } else if (!checkVarName(name)) {
        return false;
    } else if (exists(functions[getEdittingOutputFunc()].arg_names, name) ||
               exists(functions[getEdittingInputFunc()].arg_names, name)) {
        return false;
    }

    Function& func = functions[getEdittingInputFunc()];
    func.arg_types.push_back(type);
    func.default_arg_reprs.push_back(default_arg_repr);
    func.arg_names.push_back(name);
    func.default_arg_values.push_back(default_value);
    func.is_input_args.push_back(false);

    Node& node = getCurrentPipeline().nodes[InputNodeStr()];

    node.arg_reprs.push_back(default_arg_repr);
    node.links.push_back({});
    node.arg_values.push_back(default_value);

    // for current pipeline is sub pipeline.
    if (functions.count(SubPipelineFuncStr(current_pipeline))) {
        std::string f_name = SubPipelineFuncStr(current_pipeline);
        Function& sub_f = functions[f_name];

        // find input end position.
        size_t idx = getIndex(sub_f.is_input_args, false);

#define Add(container, idx, value) \
    container.insert(std::begin(container) + long(idx), value)

        Add(sub_f.arg_types, idx, type);
        Add(sub_f.default_arg_reprs, idx, default_arg_repr);
        Add(sub_f.arg_names, idx, name);
        Add(sub_f.default_arg_values, idx, default_value);
        Add(sub_f.is_input_args, idx, true);

        auto f = [&](Pipeline& pipe) {
            for (auto& pair : pipe.nodes) {
                Node& o_node = std::get<1>(pair);
                if (o_node.func_repr == f_name) {
                    Add(o_node.arg_reprs, idx, default_arg_repr);
                    Add(o_node.links, idx, Link{});
                    Add(o_node.arg_values, idx, default_value);

                    DEBUG_LOG(o_node.links.size());
                }
            }
        };

#undef Add

        for (auto& pipeline_p : pipelines) {
            DEBUG_LOG(std::get<0>(pipeline_p));
            f(std::get<1>(pipeline_p));
        }

        for (auto& pipeline_p : sub_pipelines) {
            DEBUG_LOG(std::get<0>(pipeline_p));
            f(std::get<1>(pipeline_p));
        }
    }

    version++;

    return true;
}

bool FaseCore::delInput(const size_t& idx) {
    if (idx >= functions[getEdittingInputFunc()].arg_names.size()) {
        return false;
    }

    if (is_locked_inout) {
        return false;
    }

    Node& node = getCurrentPipeline().nodes[InputNodeStr()];
    Function& func = functions[getEdittingInputFunc()];

    // store all linking info, and delete all links.
    const std::vector<std::tuple<size_t, Link>> rev_links_buf = node.rev_links;
    for (size_t i = 0; i < node.links.size(); i++) {
        delRevLink(node, i, this);
    }

    Erase(func.arg_types, idx);
    Erase(func.default_arg_reprs, idx);
    Erase(func.arg_names, idx);
    Erase(func.default_arg_values, idx);
    Erase(func.is_input_args, idx);

    Erase(node.arg_reprs, idx);
    Erase(node.links, idx);
    Erase(node.arg_values, idx);

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

    // for current pipeline is sub pipeline.
    if (functions.count(SubPipelineFuncStr(current_pipeline))) {
        auto f_name = SubPipelineFuncStr(current_pipeline);
        Function& sub_f = functions[f_name];

        Erase(sub_f.arg_types, idx);
        Erase(sub_f.default_arg_reprs, idx);
        Erase(sub_f.arg_names, idx);
        Erase(sub_f.default_arg_values, idx);
        Erase(sub_f.is_input_args, idx);

        auto f = [&](Pipeline& pipe) {
            for (auto& pair : pipe.nodes) {
                Node& o_node = std::get<1>(pair);
                if (o_node.func_repr == f_name) {
                    Erase(o_node.arg_reprs, idx);
                    Erase(o_node.links, idx);
                    Erase(o_node.arg_values, idx);
                }
            }
        };

        for (auto& pipeline_p : pipelines) {
            f(std::get<1>(pipeline_p));
        }

        for (auto& pipeline_p : sub_pipelines) {
            f(std::get<1>(pipeline_p));
        }
    }

    version++;
    return true;
}

bool FaseCore::addOutput(const std::string& name, const std::type_info* type,
                         const Variable& default_value,
                         const std::string& default_arg_repr) {
    if (is_locked_inout && !sub_pipelines.count(current_pipeline)) {
        return false;
    } else if (!checkVarName(name)) {
        return false;
    } else if (exists(functions.at(getEdittingOutputFunc()).arg_names, name) ||
               exists(functions.at(getEdittingInputFunc()).arg_names, name)) {
        return false;
    }

    Function& func = functions[getEdittingOutputFunc()];
    func.arg_types.push_back(type);
    func.default_arg_reprs.push_back(default_arg_repr);
    func.arg_names.push_back(name);
    func.default_arg_values.push_back(default_value);
    func.is_input_args.push_back(true);

    Node& node = getCurrentPipeline().nodes[OutputNodeStr()];

    node.arg_reprs.push_back(default_arg_repr);
    node.links.push_back({});
    node.arg_values.push_back(default_value);

    // for current pipeline is sub pipeline.
    if (functions.count(SubPipelineFuncStr(current_pipeline))) {
        auto f_name = SubPipelineFuncStr(current_pipeline);
        Function& sub_f = functions[f_name];

        sub_f.arg_types.emplace_back(type);
        sub_f.default_arg_reprs.emplace_back(default_arg_repr);
        sub_f.arg_names.emplace_back(name);
        sub_f.default_arg_values.emplace_back(default_value);
        sub_f.is_input_args.emplace_back(false);

        auto f = [&](Pipeline& pipe) {
            for (auto& pair : pipe.nodes) {
                if (std::get<1>(pair).func_repr == f_name) {
                    Node& o_node = std::get<1>(pair);
                    o_node.arg_reprs.push_back(default_arg_repr);
                    o_node.links.push_back({});
                    o_node.arg_values.push_back(default_value);
                    DEBUG_LOG(o_node.arg_values.size());
                }
            }
        };

        for (auto& pipeline_p : pipelines) {
            f(std::get<1>(pipeline_p));
        }

        for (auto& pipeline_p : sub_pipelines) {
            f(std::get<1>(pipeline_p));
        }
    }

    version++;
    return true;
}

bool FaseCore::delOutput(const size_t& idx) {
    if (idx >= functions[getEdittingOutputFunc()].arg_names.size()) {
        return false;
    }

    if (is_locked_inout && !sub_pipelines.count(current_pipeline)) {
        return false;
    }

    Node& node = getCurrentPipeline().nodes[OutputNodeStr()];
    Function& func = functions[getEdittingOutputFunc()];

    // store all linking info, and delete all links.
    const std::vector<Link> links_buf = node.links;
    for (size_t i = 0; i < node.links.size(); i++) {
        delLink(OutputNodeStr(), i);
    }

    Erase(func.arg_types, idx);
    Erase(func.default_arg_reprs, idx);
    Erase(func.arg_names, idx);
    Erase(func.default_arg_values, idx);
    Erase(func.is_input_args, idx);

    Erase(node.arg_reprs, idx);
    Erase(node.links, idx);
    Erase(node.arg_values, idx);

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

    // for current pipeline is sub pipeline.
    if (functions.count(SubPipelineFuncStr(current_pipeline))) {
        auto f_name = SubPipelineFuncStr(current_pipeline);
        Function& sub_f = functions[f_name];

        // find output start position.
        size_t sub_f_idx = getIndex(sub_f.is_input_args, false);

        Erase(sub_f.arg_types, sub_f_idx + idx);
        Erase(sub_f.default_arg_reprs, sub_f_idx + idx);
        Erase(sub_f.arg_names, sub_f_idx + idx);
        Erase(sub_f.default_arg_values, sub_f_idx + idx);
        Erase(sub_f.is_input_args, sub_f_idx + idx);

        auto f = [&](Pipeline& pipe) {
            for (auto& pair : pipe.nodes) {
                if (std::get<1>(pair).func_repr == f_name) {
                    Node& o_node = std::get<1>(pair);
                    Erase(o_node.arg_reprs, sub_f_idx + idx);
                    Erase(o_node.links, sub_f_idx + idx);
                    Erase(o_node.arg_values, sub_f_idx + idx);
                }
            }
        };

        for (auto& pipeline_p : pipelines) {
            f(std::get<1>(pipeline_p));
        }

        for (auto& pipeline_p : sub_pipelines) {
            f(std::get<1>(pipeline_p));
        }
    }

    return true;
}

void FaseCore::lockInOut() {
    is_locked_inout = true;
}
void FaseCore::unlockInOut() {
    is_locked_inout = false;
}

void FaseCore::switchPipeline(const std::string& project_name) {
    current_pipeline = project_name;

    version++;

    if (sub_pipelines.count(current_pipeline)) {
        return;
    } else {
        main_pipeline_last_selected = current_pipeline;
    }

    if (getCurrentPipeline().nodes.empty()) {
        initInOut();
    }

    // if there are changes of input/output func builder, fix nodes.
    // TODO FIXME it will make link bugs.
    if (getCurrentPipeline().nodes.at(InputNodeStr()).links.size() !=
        functions.at(getEdittingInputFunc()).is_input_args.size()) {
        genNode(getEdittingInputFunc(), InputNodeStr(),
                std::numeric_limits<int>::min(), &functions,
                &getCurrentPipeline().nodes);
    }
    if (getCurrentPipeline().nodes.at(OutputNodeStr()).links.size() !=
        functions.at(getEdittingOutputFunc()).is_input_args.size()) {
        genNode(getEdittingOutputFunc(), OutputNodeStr(),
                std::numeric_limits<int>::max(), &functions,
                &getCurrentPipeline().nodes);
    }
}

void FaseCore::renamePipeline(const std::string& new_name) noexcept {
    Pipeline buf = std::move(getCurrentPipeline());
    if (pipelines.count(current_pipeline)) {
        pipelines.erase(current_pipeline);
        current_pipeline = new_name;
        pipelines[new_name] = std::move(buf);
        main_pipeline_last_selected = new_name;
    } else {
        sub_pipelines.erase(current_pipeline);
        current_pipeline = new_name;
        sub_pipelines[new_name] = std::move(buf);
    }
}

void FaseCore::deletePipeline(const std::string& deleting) noexcept {
    if (current_pipeline == deleting) {
        return;
    }
    if (pipelines.count(deleting)) {
        pipelines.erase(deleting);
    } else {
        sub_pipelines.erase(deleting);
    }
}

const std::string& FaseCore::getCurrentPipelineName() const noexcept {
    return current_pipeline;
}

const std::map<std::string, Node>& FaseCore::getNodes() const {
    return getCurrentPipeline().nodes;
}

const std::map<std::string, Function>& FaseCore::getFunctions() const {
    return functions;
}

std::map<std::string, const Pipeline&> FaseCore::getPipelines() const {
    std::map<std::string, const Pipeline&> dst;
    for (const auto& p : pipelines)
        dst.insert(std::make_pair(std::get<0>(p), std::cref(std::get<1>(p))));
    for (const auto& p : sub_pipelines)
        dst.insert(std::make_pair(std::get<0>(p), std::cref(std::get<1>(p))));
    return dst;
}

std::vector<std::string> FaseCore::getPipelineNames() const {
    return getKeys(pipelines);
}

std::vector<std::string> FaseCore::getSubPipelineNames() const {
    return getKeys(sub_pipelines);
}

const std::string& FaseCore::getMainPipelineNameLastSelected() const noexcept {
    return main_pipeline_last_selected;
}

const std::vector<Variable>& FaseCore::getOutputs() const {
    return output_variables.at(OutputNodeStr());
}

int FaseCore::getVersion() const {
    return version;
}

const std::map<std::string, ResultReport>& FaseCore::run() {
    for (auto& f : built_pipeline) {
        f();
    }
    return report_box;
}

bool FaseCore::makeSubPipeline(const std::string& name) {
    if (sub_pipelines.count(name)) {
        return false;
    }

    std::function<void()> dummy = [] {};

    addFunctionBuilder(InputFuncStr(name), dummy, {}, {}, {});
    addFunctionBuilder(OutputFuncStr(name), dummy, {}, {}, {});

    // make input node
    genNode(InputFuncStr(name), InputNodeStr(), std::numeric_limits<int>::min(),
            &functions, &sub_pipelines[name].nodes);

    // make output node
    genNode(OutputFuncStr(name), OutputNodeStr(),
            std::numeric_limits<int>::max(), &functions,
            &sub_pipelines[name].nodes);

    sub_pipeline_fbs.emplace(
            name, std::make_shared<BindedPipeline>(*this, sub_pipelines[name]));

    functions.emplace(SubPipelineFuncStr(name),
                      Function{sub_pipeline_fbs[name], {}, {}, {}, {}, {}, {}});
    return true;
}

}  // namespace fase
