
#include "manager.h"

#include <iostream>
#include <map>
#include <string>

#include "constants.h"
#include "core.h"
#include "utils.h"

namespace fase {

using std::map;
using std::string;
using std::vector;

using size_t = std::size_t;

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
    class WrapedCore;

    map<string, WrapedCore> cores;
    map<string, Function> functions;

    FaildDummy dum;

    bool newPipeline(const string& name);
    bool addFunction(const string& func_name, const string& core_name);
    bool updateBindedPipes(const string& name);
    bool updateBindedPipes(WrapedCore* p);
};

class CoreManager::Impl::WrapedCore : public PipelineAPI {
public:
    WrapedCore(CoreManager::Impl& cm) : cm_ref(std::ref(cm)) {}

    bool newNode(const string& name) override {
        return core.newNode(name);
    }

    bool renameNode(const string& old_name, const string& new_name) override {
        return core.renameNode(old_name, new_name);
    }
    bool delNode(const string& name) override {
        return core.delNode(name);
    }

    bool setArgument(const string& node, size_t idx, Variable& var) override {
        return core.setArgument(node, idx, var);
    }
    bool setPriority(const string& node, int priority) override {
        return core.setPriority(node, priority);
    }

    bool allocateFunc(const string& work, const string& node) override {
        return core.allocateFunc(work, node);
    }

    bool smartLink(const string& src_node, size_t src_arg,
                   const string& dst_node, size_t dst_arg) override;
    bool unlinkNode(const string& dst_node, size_t dst_arg) override {
        return core.unlinkNode(dst_node, dst_arg);
    }

    bool supposeInput(size_t size) override {
        vector<Variable> vs(size);
        if (core.supposeInput(vs)) {
            cm_ref.get().updateBindedPipes(this);
            return true;
        }
        return false;
    }
    bool supposeOutput(size_t size) override {
        vector<Variable> vs(size);
        if (core.supposeOutput(vs)) {
            cm_ref.get().updateBindedPipes(this);
            return true;
        }
        return false;
    }

    bool run() override;

    const map<string, Node>& getNodes() const noexcept override {
        return core.getNodes();
    }

    Core core;
    std::reference_wrapper<Impl> cm_ref;
};

// ======================== WrapedCore Member Functions ========================

bool CoreManager::Impl::WrapedCore::smartLink(const string& src_node,
                                              size_t src_arg,
                                              const string& dst_node,
                                              size_t dst_arg) {
    if (core.linkNode(src_node, src_arg, dst_node, dst_arg)) return true;

    if (InputNodeName() == src_node) {
        vector<Variable> vs = core.getNodes().at(InputNodeName()).args;
        vs[src_arg] = core.getNodes().at(dst_node).args[dst_arg];
        core.supposeInput(vs);
        cm_ref.get().updateBindedPipes(this);
        return core.linkNode(src_node, src_arg, dst_node, dst_arg);

    } else if (OutputNodeName() == dst_node) {
        vector<Variable> vs = core.getNodes().at(OutputNodeName()).args;
        vs[dst_arg] = core.getNodes().at(src_node).args[src_arg];
        core.supposeOutput(vs);
        cm_ref.get().updateBindedPipes(this);
        return core.linkNode(src_node, src_arg, dst_node, dst_arg);
    }
    return false;
}

bool CoreManager::Impl::WrapedCore::run() {
    return core.run();
}

// ========================== Impl Member Functions ============================

bool CoreManager::Impl::addFunction(const string& func, const string& core) {
    vector<Variable> vs;
    RefCopy(functions[func].default_args, &vs);
    return cores.at(core).core.addUnivFunc(functions[func].func, func,
                                           std::move(vs));
}

bool CoreManager::Impl::addUnivFunc(const UnivFunc& func, const string& name,
                                    vector<Variable>&& default_args,
                                    const vector<string>& arg_names,
                                    const string& description) {
    if (cores.count(name)) return false;

    functions[name] = {
            func,
            std::move(default_args),
            {arg_names, description},
    };
    for (auto& pair : cores) {
        if (!addFunction(name, std::get<0>(pair))) return false;
    }
    return true;
}

bool CoreManager::Impl::newPipeline(const string& name) {
    if (cores.count(name) || functions.count(name)) return false;

    addUnivFunc(UnivFunc{}, name, {}, {}, "Another pipeline");

    cores.emplace(name, *this);  // create new WrapedCore.
    for (auto& pair : functions) {
        if (name != std::get<0>(pair)) {
            addFunction(std::get<0>(pair), name);
        }
    }
    return true;
}

bool CoreManager::Impl::updateBindedPipes(const string& name) {
    Function& func = functions[name];
    auto& core = cores.at(name).core;
    size_t i_size = core.getNodes().at(InputNodeName()).args.size();

    func.func = [&core, name, i_size](vector<Variable>& vs) {
        vector<Variable> inputs, outputs;
        RefCopy(vs.begin(), vs.begin() + long(i_size), &inputs);
        RefCopy(vs.begin() + long(i_size), vs.end(), &inputs);

        core.supposeInput(inputs);
        core.supposeOutput(outputs);
        core.run();
    };

    func.default_args = core.getNodes().at(InputNodeName()).args;
    func.default_args.insert(func.default_args.end() - 1,
                             core.getNodes().at(OutputNodeName()).args.begin(),
                             core.getNodes().at(OutputNodeName()).args.end());

    for (auto& pair : cores) {
        if (std::get<0>(pair) == name) {
        } else if (!addFunction(name, std::get<0>(pair))) {
            std::cerr << "CoreManager::updateBindedPipes(\"" + name +
                                 "\") : something went wrong at "
                      << std::get<0>(pair) << std::endl;
            return false;
        }
    }
    return true;
}

bool CoreManager::Impl::updateBindedPipes(WrapedCore* p) {
    for (auto& pair : cores) {
        if (&std::get<1>(pair) == p)
            return updateBindedPipes(std::get<0>(pair));
    }
    return true;
}

PipelineAPI& CoreManager::Impl::operator[](const string& name) {
    if (!cores.count(name)) {
        if (!newPipeline(name)) {
            return dum;
        }
    }
    return cores.at(name);
}

const PipelineAPI& CoreManager::Impl::operator[](const string& name) const {
    if (!cores.count(name)) {
        return dum;
    }
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
