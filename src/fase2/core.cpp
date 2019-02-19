
#include "core.h"

#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "constants.h"
#include "utils.h"

namespace fase {

constexpr char kInputFuncName[] = "FASE::InputFunc";
constexpr char kOutputFuncName[] = "FASE::OutputFunc";

using std::map, std::string, std::vector;
using size_t = std::size_t;

using Vars = vector<Variable>;

struct FuncProps {
    UnivFunc func = [](auto&, auto) {};
    Vars default_args;
};

class Core::Impl {
public:
    Impl();

    // ======= unstable API =========
    bool addUnivFunc(const UnivFunc& func, const string& name,
                     std::vector<Variable>&& default_args);

    // ======= stable API =========
    bool newNode(const string& name);
    bool renameNode(const std::string& old_name, const std::string& new_name);
    bool delNode(const string& name);

    bool setArgument(const string& node, size_t idx, Variable& var);
    bool setPriority(const std::string& node, int priority);

    bool allocateFunc(const string& work, const string& node);
    bool linkNode(const string& src_node, size_t src_arg,
                  const string& dst_node, size_t dst_arg);
    bool unlinkNode(const std::string& dst_node, std::size_t dst_arg);

    bool supposeInput(std::vector<Variable>& vars);
    bool supposeOutput(std::vector<Variable>& vars);

    bool run(Report* preport);

    const auto& getNodes() const noexcept {
        return nodes;
    }
    const std::vector<Link>& getLinks() const noexcept {
        return links;
    };

private:
    map<string, FuncProps> funcs;
    map<string, Node> nodes;

    vector<Link> links;

    Vars inputs;
    Vars outputs;

    vector<Variable>& defaultArgs(const std::string& node_name) {
        return funcs[nodes[node_name].func_name].default_args;
    }
    vector<vector<string>> getRunOrder();
    void unlinkAll(const string& node_name);
};

// ============================= Member Functions ==============================

Core::Impl::Impl() {
    funcs[kOutputFuncName];
    funcs[kInputFuncName];

    nodes[InputNodeName()] = {kInputFuncName,
                              funcs[kInputFuncName].func,
                              {},
                              std::numeric_limits<int>::max()};
    nodes[OutputNodeName()] = {kOutputFuncName,
                               funcs[kOutputFuncName].func,
                               {},
                               std::numeric_limits<int>::min()};
}

void Core::Impl::unlinkAll(const string& name) {
    for (size_t i = 0; i < nodes[name].args.size(); i++) {
        unlinkNode(name, i);
    }
    for (auto& link : links) {
        if (link.src_node == name) {
            unlinkNode(link.dst_node, link.dst_arg);
        }
    }
}

bool Core::Impl::addUnivFunc(const UnivFunc& func, const string& func_name,
                             std::vector<Variable>&& default_args) {
    funcs[func_name] = {func, std::forward<vector<Variable>>(default_args)};

    for (auto& [node_name, node] : nodes) {
        if (node.func_name == func_name) {
            allocateFunc(func_name, node_name);
        }
    }
    return true;
}

bool Core::Impl::newNode(const string& name) {
    if (nodes.count(name)) {
        return false;
    }
    nodes[name];
    return true;
}

bool Core::Impl::renameNode(const std::string& old_name,
                            const std::string& new_name) {
    if (!nodes.count(old_name) || old_name == InputNodeName() ||
        old_name == OutputNodeName()) {
        return false;
    }
    nodes[new_name] = std::move(nodes[old_name]);
    for (auto& link : links) {
        if (link.dst_node == old_name) link.dst_node = new_name;
        if (link.src_node == old_name) link.src_node = new_name;
    }
    return true;
}

bool Core::Impl::delNode(const string& name) {
    if (nodes.count(name) && name != InputNodeName() &&
        name != OutputNodeName()) {
        unlinkAll(name);
        nodes.erase(name);
        return true;
    }
    return false;
}

bool Core::Impl::setArgument(const string& node, size_t idx, Variable& var) {
    if (idx >= nodes.size() || !defaultArgs(node)[idx].isSameType(var)) {
        return false;
    }
    nodes[node].args[idx] = var.ref();
    return true;
}

bool Core::Impl::setPriority(const string& node, int priority) {
    if (nodes.count(node)) {
        nodes[node].priority = priority;
        return true;
    }
    return false;
}

bool Core::Impl::allocateFunc(const string& func, const string& node_name) {
    if (funcs.count(func)) {
        auto& node = nodes[node_name];
        node.func_name = func;
        node.args = funcs[func].default_args;
        node.func = funcs[func].func;

        auto link_bufs = get_all_if(links, [n = node_name](auto& l) {
            return l.src_node == n || l.dst_node == n;
        });
        unlinkAll(node_name);
        for (auto& s : link_bufs) {
            linkNode(s.src_node, s.src_arg, s.dst_node, s.dst_arg);
        }
        return true;
    }
    return false;
}

bool Core::Impl::linkNode(const string& src_node, size_t s_idx,
                          const string& dst_node, size_t d_idx) {
    if (defaultArgs(src_node)[s_idx].isSameType(defaultArgs(dst_node)[d_idx])) {
        unlinkNode(dst_node, d_idx);
        links.emplace_back(Link{src_node, s_idx, dst_node, d_idx});
        return true;
    }
    return false;
}

bool Core::Impl::unlinkNode(const std::string& dst_node, std::size_t dst_arg) {
    for (size_t i = 0; i < links.size(); i++) {
        if (links[i].dst_node == dst_node && links[i].dst_arg == dst_arg) {
            links.erase(links.begin() + long(i));
            return true;
        }
    }
    return false;
}

bool Core::Impl::supposeInput(std::vector<Variable>& vars) {
    RefCopy(vars, &inputs);
    nodes[InputNodeName()].args = inputs;
    defaultArgs(InputNodeName()) = vars;
    return true;
}

bool Core::Impl::supposeOutput(std::vector<Variable>& vars) {
    RefCopy(vars, &outputs);
    RefCopy(vars, &nodes[OutputNodeName()].args);
    defaultArgs(OutputNodeName()) = vars;
    return true;
}

vector<vector<string>> Core::Impl::getRunOrder() {
    vector<string> dones = {InputNodeName()};
    vector<vector<string>> dst = {dones};
    auto get_size = [](auto& double_array) {
        size_t s = 0;
        for (auto& d : double_array) s += d.size();
        return s;
    };

    while (1) {
        int max_priority = std::numeric_limits<int>::min();
        vector<string> runnables;
        for (auto& [name, node] : nodes) {
            if (exsists(name, dones)) {
                continue;
            }

            bool ok_f = true;
            for (auto& link : links) {
                if (link.dst_node == name && !exsists(link.src_node, dones)) {
                    ok_f = false;
                    break;
                }
            }
            if (ok_f) {
                if (node.priority > max_priority) {
                    runnables.clear();
                    max_priority = node.priority;
                }
                if (node.priority >= max_priority) {
                    runnables.push_back(name);
                }
            }
        }

        if (runnables.size() == 0) {
            return {};
        }

        dones.insert(dones.begin(), runnables.begin(), runnables.end());
        dst.emplace_back(std::move(runnables));

        if (get_size(dst) == nodes.size()) {
            return dst;
        }
    }
}

bool Core::Impl::run(Report* preport) {
    vector<vector<string>> order = getRunOrder();
    if (order.size() == 0) {
        return false;
    }

    nodes[InputNodeName()].args = inputs;
    nodes[OutputNodeName()].args.resize(outputs.size());

    for (auto& link : links) {
        nodes[link.dst_node].args[link.dst_arg] =
                nodes[link.src_node].args[link.src_arg].ref();
    }

    for (auto& node_names : order) {
        for (auto& node_name : node_names) {
            Report* p = nullptr;
            if (preport != nullptr) {
                p = &preport->child_reports[node_name];
            }
            nodes[node_name].func(nodes[node_name].args, p);
        }
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        nodes[OutputNodeName()].args[i].copyTo(outputs[i]);
    }
    return true;
}

// ============================== Pimpl Pattern ================================

Core::Core() : pimpl(std::make_unique<Impl>()) {}
Core::~Core() = default;

// ======= unstable API =========
bool Core::addUnivFunc(const UnivFunc& func, const string& name,
                       std::vector<Variable>&& default_args) {
    return pimpl->addUnivFunc(
            func, name, std::forward<std::vector<Variable>>(default_args));
}

// ======= stable API =========
bool Core::newNode(const string& name) {
    return pimpl->newNode(name);
}
bool Core::renameNode(const std::string& old_name,
                      const std::string& new_name) {
    return pimpl->renameNode(old_name, new_name);
}
bool Core::delNode(const string& name) {
    return pimpl->delNode(name);
}

bool Core::setArgument(const string& node, size_t idx, Variable& var) {
    return pimpl->setArgument(node, idx, var);
}

bool Core::setPriority(const std::string& node, int priority) {
    return pimpl->setPriority(node, priority);
}

bool Core::allocateFunc(const string& work, const string& node) {
    return pimpl->allocateFunc(work, node);
}

bool Core::linkNode(const string& src_node, size_t src_arg,
                    const string& dst_node, size_t dst_arg) {
    return pimpl->linkNode(src_node, src_arg, dst_node, dst_arg);
}

bool Core::unlinkNode(const std::string& dst_node, std::size_t dst_arg) {
    return pimpl->unlinkNode(dst_node, dst_arg);
}

bool Core::supposeInput(std::vector<Variable>& vars) {
    return pimpl->supposeInput(vars);
}
bool Core::supposeOutput(std::vector<Variable>& vars) {
    return pimpl->supposeOutput(vars);
}

bool Core::run(Report* preport) {
    return pimpl->run(preport);
}

const std::map<std::string, Node>& Core::getNodes() const noexcept {
    return pimpl->getNodes();
}

const std::vector<Link>& Core::getLinks() const noexcept {
    return pimpl->getLinks();
}

}  // namespace fase
