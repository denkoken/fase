
#include "core.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include <future>

#include "constants.h"
#include "utils.h"

namespace fase {

using std::map, std::string, std::vector;
using size_t = std::size_t;

using Vars = vector<Variable>;

namespace {

template <typename Task>
bool WrapError(const std::string& n_name, Task&& task) {
    try {
        task();
    } catch (WrongTypeCast&) {
        std::cerr << "Core::run() : something went wrong (type) at " << n_name
                  << ". fix bug of Fase." << std::endl;
        return false;
    } catch (TryToGetEmptyVariable&) {
        std::cerr << "Core::run() : something went wrong (empty) at " << n_name
                  << ". fix bug of Fase." << std::endl;
        return false;
    } catch (ErrorThrownByNode& e) {
        try {
            e.rethrow_nested();
        } catch (...) {
            throw(ErrorThrownByNode(n_name + " :: " + e.node_name));
        }
    } catch (...) {
        throw(ErrorThrownByNode(n_name));
    }
    return true;
}

}  // namespace

vector<vector<string>> GetRunOrder(const map<string, Node>& nodes,
                                   const vector<Link>& links) {
    vector<string> dones = {InputNodeName()};
    vector<vector<string>> dst = {dones};

    while (1) {
        int max_priority = std::numeric_limits<int>::min();
        vector<string> runnables;
        for (auto& [n_name, node] : nodes) {
            if (exists(n_name, dones)) {
                continue;
            }

            bool ok_f = true;
            for (auto& link : links) {
                if (link.dst_node == n_name && !exists(link.src_node, dones)) {
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
                    runnables.push_back(n_name);
                }
            }
        }

        if (runnables.size() == 0) {
            return {};
        }

        dones.insert(dones.begin(), runnables.begin(), runnables.end());
        dst.emplace_back(std::move(runnables));

        if (dones.size() == nodes.size()) {
            return dst;
        }
    }
}

struct FuncProps {
    UnivFunc func = [](auto&, auto) {};
    Vars default_args;
};

class Core::Impl {
public:
    Impl();

    // ======= unstable API =========
    bool addUnivFunc(const UnivFunc& func, const string& f_name,
                     std::vector<Variable>&& default_args);

    // ======= stable API =========
    bool newNode(const string& n_name);
    bool renameNode(const std::string& old_n_name,
                    const std::string& new_n_name);
    bool delNode(const string& n_name);

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

    vector<Variable>& defaultArgs(const std::string& n_name) {
        return funcs[nodes[n_name].func_name].default_args;
    }
    void unlinkAll(const string& n_name);
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

void Core::Impl::unlinkAll(const string& n_name) {
    for (size_t i = 0; i < nodes[n_name].args.size(); i++) {
        unlinkNode(n_name, i);
    }
    for (auto& link : links) {
        if (link.src_node == n_name) {
            unlinkNode(link.dst_node, link.dst_arg);
        }
    }
}

bool Core::Impl::addUnivFunc(const UnivFunc& func, const string& f_name,
                             std::vector<Variable>&& default_args) {
    funcs[f_name] = {func, std::forward<vector<Variable>>(default_args)};

    for (auto& [node_name, node] : nodes) {
        if (node.func_name == f_name) {
            allocateFunc(f_name, node_name);
        }
    }
    return true;
}

bool Core::Impl::newNode(const string& n_name) {
    if (nodes.count(n_name) || n_name.empty()) {
        return false;
    }
    nodes[n_name];
    return true;
}

bool Core::Impl::renameNode(const std::string& old_n_name,
                            const std::string& new_n_name) {
    if (!nodes.count(old_n_name) || old_n_name == InputNodeName() ||
        old_n_name == OutputNodeName() || new_n_name.empty()) {
        return false;
    }
    nodes[new_n_name] = std::move(nodes[old_n_name]);
    for (auto& link : links) {
        if (link.dst_node == old_n_name) link.dst_node = new_n_name;
        if (link.src_node == old_n_name) link.src_node = new_n_name;
    }
    delNode(old_n_name);
    return true;
}

bool Core::Impl::delNode(const string& n_name) {
    if (nodes.count(n_name) && n_name != InputNodeName() &&
        n_name != OutputNodeName()) {
        unlinkAll(n_name);
        nodes.erase(n_name);
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
    if (node_name == InputNodeName() || node_name == OutputNodeName()) {
        return false;
    }
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

bool Core::Impl::linkNode(const string& s_n_name, size_t s_idx,
                          const string& d_n_name, size_t d_idx) {
    if (nodes[s_n_name].args.size() <= s_idx ||
        nodes[d_n_name].args.size() <= d_idx) {
        return false;
    }
    if (!defaultArgs(s_n_name)[s_idx].isSameType(
                defaultArgs(d_n_name)[d_idx])) {
        return false;
    }
    unlinkNode(d_n_name, d_idx);
    links.emplace_back(Link{s_n_name, s_idx, d_n_name, d_idx});
    if (GetRunOrder(nodes, links).empty()) {
        links.pop_back();
        return false;
    }
    return true;
}

bool Core::Impl::unlinkNode(const std::string& dst_n_name,
                            std::size_t dst_arg) {
    for (size_t i = 0; i < links.size(); i++) {
        if (links[i].dst_node == dst_n_name && links[i].dst_arg == dst_arg) {
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

bool Core::Impl::run(Report* preport) {
    vector<vector<string>> order = GetRunOrder(nodes, links);
    if (order.size() == 0) {
        return false;
    }

    nodes[InputNodeName()].args = inputs;
    nodes[OutputNodeName()].args.resize(outputs.size());

    auto compare_node = [&](auto& a, auto& b) -> int {
        if (a == b) return 0;
        for (auto& layer : order) {
            for (auto& n_name : layer) {
                if (n_name == a) {
                    return 1;
                } else if (n_name == b) {
                    return -1;
                }
            }
        }
        assert(false);
    };

    // sort link objects.
    auto compare = [&](const Link& l1, const Link& l2) -> bool {
        int ret = compare_node(l1.src_node, l2.src_node);
        assert(-1);
        if (ret) return ret > 0;
        if (l1.src_arg != l2.src_arg) {
            return l1.src_arg < l2.src_arg;
        }
        ret = compare_node(l1.dst_node, l2.dst_node);
        if (ret) return ret > 0;
        if (l1.dst_arg != l2.dst_arg) {
            return l1.dst_arg < l2.dst_arg;
        }
        assert(false);
    };
    size_t a = links.size();
    printf("before num of links: %lu \n", links.size());
    std::sort(links.begin(), links.end(), compare);
    printf("after num of links: %lu \n", links.size());
    assert(a == links.size());

    // link node args.
    for (auto& link : links) {
        printf("%s : %lu -> %s : %lu\n", link.src_node.c_str(), link.src_arg,
               link.dst_node.c_str(), link.dst_arg);

        nodes[link.dst_node].args[link.dst_arg] =
                nodes[link.src_node].args[link.src_arg].ref();
    }

    auto start = std::chrono::system_clock::now();
#if 1
    for (auto& node_names : order) {
        for (auto& n_name : node_names) {
            Report* p = nullptr;
            if (preport != nullptr) {
                p = &preport->child_reports[n_name];
            }
            if (!WrapError(n_name, [&]() {
                    nodes[n_name].func(nodes[n_name].args, p);
                })) {
                return false;
            }
        }
    }
#else
    std::launch policy = std::launch::async | std::launch::deferred;

    for (auto& node_names : order) {
        map<string, std::future<void>> futures;
        for (auto& n_name : node_names) {
            Report* p = nullptr;
            if (preport != nullptr) {
                p = &preport->child_reports[n_name];
            }
            futures[n_name] = std::async(policy, [&]() {
                nodes[n_name].func(nodes[n_name].args, p);
            });
        }
        for (auto& n_name : node_names) {
            if (!WrapError(n_name, [&]() { futures[n_name].wait(); })) {
                return false;
            }
        }
    }
#endif
    if (preport != nullptr) {
        preport->execution_time = std::chrono::system_clock::now() - start;
    }

    for (size_t i = 0; i < outputs.size(); i++) {
        nodes[OutputNodeName()].args[i].copyTo(outputs[i]);
    }
    return true;
}

// ============================== Pimpl Pattern ================================

Core::Core() : pimpl(std::make_unique<Impl>()) {}
Core::Core(Core& o) : pimpl(std::make_unique<Impl>(*o.pimpl)) {}
Core::Core(const Core& o) : pimpl(std::make_unique<Impl>(*o.pimpl)) {}
Core::Core(Core&& o) : pimpl(std::move(o.pimpl)) {}
Core& Core::operator=(Core& o) {
    pimpl = std::make_unique<Impl>(*o.pimpl);
    return *this;
}
Core& Core::operator=(const Core& o) {
    pimpl = std::make_unique<Impl>(*o.pimpl);
    return *this;
}
Core& Core::operator=(Core&& o) {
    pimpl = std::move(o.pimpl);
    return *this;
}
Core::~Core() = default;

// ======= unstable API =========
bool Core::addUnivFunc(const UnivFunc& func, const string& f_name,
                       std::vector<Variable>&& default_args) {
    return pimpl->addUnivFunc(
            func, f_name, std::forward<std::vector<Variable>>(default_args));
}

// ======= stable API =========
bool Core::newNode(const string& n_name) {
    return pimpl->newNode(n_name);
}
bool Core::renameNode(const std::string& old_n_name,
                      const std::string& new_n_name) {
    return pimpl->renameNode(old_n_name, new_n_name);
}
bool Core::delNode(const string& n_name) {
    return pimpl->delNode(n_name);
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
