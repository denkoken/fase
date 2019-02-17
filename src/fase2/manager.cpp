
#include "manager.h"

#include <map>
#include <string>

#include "core.h"
#include "utils.h"

namespace fase {

using std::map;
using std::string;
using std::vector;

using size_t = size_t;

class WrapedCore : public PipelineAPI {
public:
    bool newNode(const string& name) override;
    bool renameNode(const string& old_name, const string& new_name) override;
    bool delNode(const string& name) override;

    bool setArgument(const string& node, size_t idx, Variable& var) override;
    bool setPriority(const string& node, int priority) override;

    bool allocateFunc(const string& work, const string& node) override;

    bool smartLink(const string& src_node, size_t src_arg,
                   const string& dst_node, size_t dst_arg) override;
    bool unlinkNode(const string& dst_node, size_t dst_arg) override;

    bool supposeInput(size_t size) override;
    bool supposeOutput(size_t size) override;

    bool run() override;

    const map<string, Node>& getNodes() const noexcept override;

    Core core;
};

struct Function {
    UnivFunc func;
    vector<Variable> default_args;
    FunctionUtils utils;
};

class CoreManager::Impl {
public:
    bool addUnivFunc(const UnivFunc& func, const string& name,
                     vector<Variable>&& default_args,
                     const vector<string>& arg_names,
                     const std::string& description);

    PipelineAPI& operator[](const string& core_name);
    const PipelineAPI& operator[](const string& name) const;

    vector<string> getPipelineNames();
    map<string, FunctionUtils> getFunctionUtils();

private:
    map<string, WrapedCore> cores;
    map<string, Function> functions;
};

// ======================== WrapedCore Member Functions ========================

bool WrapedCore::newNode(const string& name) {
    return core.newNode(name);
}

bool WrapedCore::renameNode(const string& old_name, const string& new_name) {
    return core.renameNode(old_name, new_name);
}

bool WrapedCore::delNode(const string& name) {
    return core.delNode(name);
}

bool WrapedCore::setArgument(const string& node, size_t idx, Variable& var) {
    return core.setArgument(node, idx, var);
}

bool WrapedCore::setPriority(const string& node, int priority) {
    return core.setPriority(node, priority);
}

bool WrapedCore::allocateFunc(const string& work, const string& node) {
    return core.allocateFunc(work, node);
}

bool WrapedCore::smartLink(const string& src_node, size_t src_arg,
                           const string& dst_node, size_t dst_arg) {
    // TODO
    return core.linkNode(src_node, src_arg, dst_node, dst_arg);
}
bool WrapedCore::unlinkNode(const string& dst_node, size_t dst_arg) {
    return core.unlinkNode(dst_node, dst_arg);
}

bool WrapedCore::supposeInput(size_t size) {
    vector<Variable> vs(size);
    return core.supposeInput(vs);
}
bool WrapedCore::supposeOutput(size_t size) {
    vector<Variable> vs(size);
    return core.supposeOutput(vs);
}

bool WrapedCore::run() {
    // TODO
}

const map<string, Node>& WrapedCore::getNodes() const noexcept {
    return core.getNodes();
}

// ========================== Impl Member Functions ============================

bool CoreManager::Impl::addUnivFunc(const UnivFunc& func, const string& name,
                                    vector<Variable>&& default_args,
                                    const vector<string>& arg_names,
                                    const string& description) {
    if (cores.count(name)) {
        return false;
    }
    functions[name] = {
            func,
            std::move(default_args),
            {arg_names, description},
    };
    for (auto& pair : cores) {
        vector<Variable> vs;
        RefCopy(functions[name].default_args, &vs);
        if (!std::get<1>(pair).core.addUnivFunc(func, name, std::move(vs))) {
            return false;
        }
    }
    return true;
}

PipelineAPI& CoreManager::Impl::operator[](const string& core_name) {
    if (!cores.count(core_name)) {
        cores[core_name];  // create new WrapedCore.
        for (auto& pair : functions) {
            auto& name = std::get<0>(pair);
            auto& func = std::get<1>(pair);
            vector<Variable> vs;
            RefCopy(functions[name].default_args, &vs);
            cores[core_name].core.addUnivFunc(func.func, name, std::move(vs));
        }
    }
    return cores[core_name];
}

const PipelineAPI& CoreManager::Impl::operator[](const string& name) const {
    return cores.at(name);
}

vector<string> CoreManager::Impl::getPipelineNames() {
    vector<string> dst;
    for (auto& pair : cores) {
        dst.emplace_back(std::get<0>(pair));
    }
    return dst;
}

map<string, FunctionUtils> CoreManager::Impl::getFunctionUtils() {
    map<string, FunctionUtils> dst;
    for (auto& pair : functions) {
        dst[std::get<0>(pair)] = std::get<1>(pair).utils;
    }
    return dst;
}

// ============================== Pimpl Pattern ================================

CoreManager::CoreManager() : pimpl(std::make_unique<Impl>()) {}
CoreManager::~CoreManager() = default;

bool CoreManager::addUnivFunc(const UnivFunc& func, const string& name,
                              vector<Variable>&& default_args,
                              const vector<string>& arg_names,
                              const std::string& description) {
    return pimpl->addUnivFunc(func, name, move(default_args), arg_names,
                              description);
}

PipelineAPI& CoreManager::operator[](const string& name) {
    return (*pimpl)[name];
}
const PipelineAPI& CoreManager::operator[](const string& name) const {
    return (*pimpl)[name];
}

vector<string> CoreManager::getPipelineNames() {
    return pimpl->getPipelineNames();
}

map<string, FunctionUtils> CoreManager::getFunctionUtils() {
    return pimpl->getFunctionUtils();
}

}  // namespace fase
