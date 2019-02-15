
#include "core.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "constants.h"

namespace fase {

using std::begin;
using std::end;
using std::map;
using std::size_t;
using std::string;
using std::vector;

using Vars = vector<Variable>;

namespace {

template <typename T, typename C>
bool exsists(T& t, C& c) {
    return end(c) != std::find(begin(c), end(c), t);
}

}  // namespace

struct Node {
    string worker_name;
    vector<Variable> args;
    int priority;
};

struct Link {
    string src_node;
    size_t src_arg;
    string dst_node;
    size_t dst_arg;
};

struct WorkProp {
    UnivFunc worker;
    Vars default_args;
};

class Core::Impl {
public:
    Impl();

    // ======= unstable API =========
    bool addUnivFunc(UnivFunc&& worker, const string& name,
                     std::vector<Variable>&& default_args);

    // ======= stable API =========
    bool newNode(const string& name) noexcept;
    bool delNode(const string& name) noexcept;

    bool setArgument(const string& node, size_t idx, Variable& var);
    bool setPriority(const std::string& node, int priority);

    bool allocateWork(const string& work, const string& node);
    bool linkNode(const string& src_node, size_t src_arg,
                  const string& dst_node, size_t dst_arg);
    bool unlinkNode(const std::string& dst_node, std::size_t dst_arg);

    bool supposeInput(std::vector<Variable>& vars);
    bool supposeOutput(std::vector<Variable>& vars);

    bool run();

private:
    map<string, WorkProp> workers;
    map<string, Node> nodes;

    vector<Link> links;

    Vars inputs;
    Vars outputs;

    vector<string> getRunOrder();
};

// ============================= Member Functions ==============================

Core::Impl::Impl() {
    nodes[InputNodeName()];
    nodes[OutputNodeName()];
    workers[""] = {UnivFunc([](auto&) {}), {}};
}

bool Core::Impl::addUnivFunc(UnivFunc&& worker, const string& name,
                             std::vector<Variable>&& default_args) {
    workers[name] = {std::forward<UnivFunc>(worker),
                     std::forward<vector<Variable>>(default_args)};
    return true;
}

bool Core::Impl::newNode(const string& name) noexcept {
    if (nodes.count(name)) {
        return false;
    }
    nodes[name];
    return true;
}

bool Core::Impl::delNode(const string& name) noexcept {
    return nodes.size() != nodes.erase(name);
}

bool Core::Impl::setArgument(const string& node, size_t idx, Variable& var) {
    if (idx >= nodes.size()) {
        return false;
    }
    nodes[node].args[idx] = var.ref();
    return true;
}

bool Core::Impl::setPriority(const string& node, int priority) {
    if (nodes.count(node)) {
        return false;
    }

    nodes[node].priority = priority;
    return true;
}

bool Core::Impl::allocateWork(const string& work, const string& node) {
    if (workers.count(work)) {
        return false;
    }
    nodes[node].worker_name = work;
    nodes[node].args.resize(workers[work].default_args.size());
    return true;
}

bool Core::Impl::linkNode(const string& src_node, size_t src_arg,
                          const string& dst_node, size_t dst_arg) {
    unlinkNode(dst_node, dst_arg);
    links.emplace_back(Link{src_node, src_arg, dst_node, dst_arg});
    return true;
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
    inputs.clear();
    inputs.resize(vars.size());
    for (size_t i = 0; i < vars.size(); i++) {
        inputs[i] = vars[i].ref();
    }
    nodes[InputNodeName()].args = inputs;
    return true;
}

bool Core::Impl::supposeOutput(std::vector<Variable>& vars) {
    outputs.clear();
    outputs.resize(vars.size());
    for (size_t i = 0; i < vars.size(); i++) {
        outputs[i] = vars[i].ref();
    }
    return true;
}

vector<string> Core::Impl::getRunOrder() {
    vector<string> dones = {InputNodeName()};

    while (1) {
        int max_priority = std::numeric_limits<int>::min();
        vector<string> runnables;
        for (const auto& pair : nodes) {
            auto& name = std::get<0>(pair);
            auto& node = std::get<1>(pair);

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
                }
                if (node.priority >= max_priority) {
                    runnables.push_back(name);
                }
            }
        }

        for (auto& a : runnables) {
            dones.push_back(a);
        }

        if (dones.size() == nodes.size()) {
            break;
        }

        if (runnables.size() == 0) {
            return {};
        }
    }

    return {begin(dones) + 1, end(dones)};
}

bool Core::Impl::run() {
    vector<string> order = getRunOrder();
    if (order.size() == 0) {
        return false;
    }

    nodes[InputNodeName()].args.resize(inputs.size());
    nodes[OutputNodeName()].args.resize(outputs.size());

    for (size_t i = 0; i < inputs.size(); i++) {
        inputs[i] = nodes[InputNodeName()].args[i].clone();
    }

    for (auto& link : links) {
        nodes[link.dst_node].args[link.dst_arg] =
                nodes[link.src_node].args[link.src_arg].ref();
    }

    for (auto& name : order) {
        workers[nodes[name].worker_name].worker(nodes[name].args);
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
bool Core::addUnivFunc(UnivFunc&& worker, const string& name,
                       std::vector<Variable>&& default_args) {
    return pimpl->addUnivFunc(
            std::forward<UnivFunc>(worker), name,
            std::forward<std::vector<Variable>>(default_args));
}

// ======= stable API =========
bool Core::newNode(const string& name) noexcept {
    return pimpl->newNode(name);
}
bool Core::delNode(const string& name) noexcept {
    return pimpl->delNode(name);
}

bool Core::setArgument(const string& node, size_t idx, Variable& var) {
    return pimpl->setArgument(node, idx, var);
}

bool Core::allocateWork(const string& work, const string& node) {
    return pimpl->allocateWork(work, node);
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

bool Core::run() {
    return pimpl->run();
}

bool Core::setPriority(const std::string& node, int priority) {
    return pimpl->setPriority(node, priority);
}

}  // namespace fase
