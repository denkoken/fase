
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
    Impl() = default;
    ~Impl() = default;
    bool addUnivFunc(const UnivFunc& func, const string& f_name,
                     vector<Variable>&& default_args,
                     const vector<string>& arg_names,
                     const std::string& description);

    PipelineAPI& operator[](const string& c_name);
    const PipelineAPI& operator[](const string& c_name) const;

    vector<string> getPipelineNames();
    map<string, FunctionUtils> getFunctionUtils();

private:
    class WrapedCore;

    map<string, WrapedCore> cores;
    map<string, Function> functions;

    DependenceTree dependence_tree;

    FaildDummy dum;

    bool newPipeline(const string& c_name);
    bool addFunction(const string& f_name, const string& c_name);
    bool updateBindedPipes(const string& c_name);
};

class CoreManager::Impl::WrapedCore : public PipelineAPI {
public:
    WrapedCore(CoreManager::Impl& cm) : cm_ref(std::ref(cm)) {}
    ~WrapedCore() = default;

    bool newNode(const string& n_name) override {
        return core.newNode(n_name);
    }

    bool renameNode(const string& old_n_name,
                    const string& new_n_name) override {
        return core.renameNode(old_n_name, new_n_name);
    }
    bool delNode(const string& n_name) override {
        auto& d_tree = cm_ref.get().dependence_tree;
        d_tree.del(myname(), core.getNodes().at(n_name).func_name);
        return core.delNode(n_name);
    }

    bool setArgument(const string& n_name, size_t idx, Variable& var) override {
        return core.setArgument(n_name, idx, var);
    }
    bool setPriority(const string& n_name, int priority) override {
        return core.setPriority(n_name, priority);
    }

    bool allocateFunc(const string& f_name, const string& n_name) override {
        auto& d_tree = cm_ref.get().dependence_tree;
        if (cm_ref.get().cores.count(core.getNodes().at(n_name).func_name)) {
            d_tree.del(myname(), core.getNodes().at(n_name).func_name);
        }
        if (d_tree.add(myname(), f_name)) {
            return core.allocateFunc(f_name, n_name);
        }
        return false;
    }

    bool smartLink(const string& src_node, size_t src_arg,
                   const string& dst_node, size_t dst_arg) override;
    bool unlinkNode(const string& dst_node, size_t dst_arg) override {
        return core.unlinkNode(dst_node, dst_arg);
    }

    bool supposeInput(const std::vector<std::string>& arg_names) override {
        input_var_names = arg_names;
        inputs.resize(arg_names.size());
        if (core.supposeInput(inputs)) {
            cm_ref.get().updateBindedPipes(myname());
            return true;
        }
        return false;
    }

    bool supposeOutput(const std::vector<std::string>& arg_names) override {
        output_var_names = arg_names;
        outputs.resize(arg_names.size());
        if (core.supposeOutput(outputs)) {
            cm_ref.get().updateBindedPipes(myname());
            return true;
        }
        return false;
    }

    bool run(Report* preport = nullptr) override;

    const map<string, Node>& getNodes() const noexcept override {
        return core.getNodes();
    }
    const vector<Link>& getLinks() const noexcept override {
        return core.getLinks();
    }

    Core core;
    std::reference_wrapper<Impl> cm_ref;

    vector<Variable> inputs;
    vector<Variable> outputs;
    vector<string> input_var_names;
    vector<string> output_var_names;

    const string& myname() {
        for (auto& [c_name, wraped] : cm_ref.get().cores) {
            if (&wraped == this) return c_name;
        }
        throw std::logic_error("myname is not found!");
    }
};

// ======================== WrapedCore Member Functions ========================

bool CoreManager::Impl::WrapedCore::smartLink(const string& src_node,
                                              size_t src_arg,
                                              const string& dst_node,
                                              size_t dst_arg) {
    if (core.getNodes().at(src_node).args.size() <= src_arg) return false;
    if (core.getNodes().at(dst_node).args.size() <= dst_arg) return false;
    if (core.linkNode(src_node, src_arg, dst_node, dst_arg)) return true;

    if (InputNodeName() == src_node) {
        inputs[src_arg] = core.getNodes().at(dst_node).args[dst_arg];
        core.supposeInput(inputs);
        cm_ref.get().updateBindedPipes(myname());
        return core.linkNode(src_node, src_arg, dst_node, dst_arg);

    } else if (OutputNodeName() == dst_node) {
        outputs[dst_arg] = core.getNodes().at(src_node).args[src_arg];
        core.supposeOutput(outputs);
        cm_ref.get().updateBindedPipes(myname());
        return core.linkNode(src_node, src_arg, dst_node, dst_arg);
    }
    return false;
}

bool CoreManager::Impl::WrapedCore::run(Report* preport) {
    return core.run(preport);
}

// ========================== Impl Member Functions ============================

bool CoreManager::Impl::addFunction(const string& func, const string& core) {
    vector<Variable> vs;
    RefCopy(functions[func].default_args, &vs);
    return cores.at(core).core.addUnivFunc(functions[func].func, func,
                                           std::move(vs));
}

bool CoreManager::Impl::addUnivFunc(const UnivFunc& func, const string& f_name,
                                    vector<Variable>&& default_args,
                                    const vector<string>& arg_names,
                                    const string& description) {
    if (cores.count(f_name)) return false;

    functions[f_name] = {
            func,
            std::move(default_args),
            {arg_names, description},
    };
    for (auto& [c_name, wraped] : cores) {
        if (!addFunction(f_name, c_name)) return false;
    }
    return true;
}

bool CoreManager::Impl::newPipeline(const string& c_name) {
    if (cores.count(c_name) || functions.count(c_name)) return false;

    addUnivFunc(UnivFunc{}, c_name, {}, {}, "Another pipeline");

    cores.emplace(c_name, *this);  // create new WrapedCore.
    for (auto& [f_name, func] : functions) {
        if (c_name != f_name) {
            addFunction(f_name, c_name);
        }
    }
    return true;
}

bool CoreManager::Impl::updateBindedPipes(const string& c_name) {
    Function& func = functions[c_name];
    auto& core = cores.at(c_name).core;

    // Update Function::func (UnivFunc)
    func.func = [&core, c_name](vector<Variable>& vs, Report* preport) {
        size_t i_size = core.getNodes().at(InputNodeName()).args.size();
        size_t o_size = core.getNodes().at(OutputNodeName()).args.size();
        if (vs.size() != i_size + o_size) {
            throw std::logic_error("Invalid size of variables at Binded Pipe.");
        }
        vector<Variable> inputs, outputs;
        RefCopy(vs.begin(), vs.begin() + long(i_size), &inputs);
        RefCopy(vs.begin() + long(i_size), vs.end(), &outputs);

        core.supposeInput(inputs);
        core.supposeOutput(outputs);
        if (!core.run(preport)) {
            throw(std::runtime_error(c_name + " is failed!"));
        }
    };

    // Update Function::default_args.
    size_t i_size = core.getNodes().at(InputNodeName()).args.size();
    RefCopy(cores.at(c_name).inputs, &func.default_args);
    func.default_args.resize(i_size + cores.at(c_name).outputs.size());
    for (size_t i = 0; i < cores.at(c_name).outputs.size(); i++) {
        func.default_args[i + i_size] = cores.at(c_name).outputs[i].ref();
    }

    // Update Function::utils::arg_names.
    func.utils.arg_names = cores.at(c_name).input_var_names;
    func.utils.arg_names.insert(func.utils.arg_names.end() - 1,
                                cores.at(c_name).output_var_names.begin(),
                                cores.at(c_name).output_var_names.end());

    // Add updated function to other pipelines.
    for (auto& [other_c_name, wraped] : cores) {
        if (other_c_name == c_name) {
        } else if (!addFunction(c_name, other_c_name)) {
            std::cerr << "CoreManager::updateBindedPipes(\"" + c_name +
                                 "\") : something went wrong at "
                      << other_c_name << std::endl;
            return false;
        }
    }
    return true;
}

PipelineAPI& CoreManager::Impl::operator[](const string& c_name) {
    if (!cores.count(c_name)) {
        if (!newPipeline(c_name)) {
            return dum;
        }
    }
    return cores.at(c_name);
}

const PipelineAPI& CoreManager::Impl::operator[](const string& c_name) const {
    if (!cores.count(c_name)) {
        return dum;
    }
    return cores.at(c_name);
}

vector<string> CoreManager::Impl::getPipelineNames() {
    vector<string> dst;
    for (auto& [c_name, wraped] : cores) {
        dst.emplace_back(c_name);
    }
    return dst;
}

map<string, FunctionUtils> CoreManager::Impl::getFunctionUtils() {
    map<string, FunctionUtils> dst;
    for (auto& [f_name, func] : functions) {
        dst[f_name] = func.utils;
    }
    return dst;
}

// ============================== Pimpl Pattern ================================

CoreManager::CoreManager() : pimpl(std::make_unique<Impl>()) {}
CoreManager::~CoreManager() = default;

bool CoreManager::addUnivFunc(const UnivFunc& func, const string& c_name,
                              vector<Variable>&& default_args,
                              const vector<string>& arg_names,
                              const std::string& description) {
    return pimpl->addUnivFunc(func, c_name, move(default_args), arg_names,
                              description);
}

PipelineAPI& CoreManager::operator[](const string& c_name) {
    return (*pimpl)[c_name];
}
const PipelineAPI& CoreManager::operator[](const string& c_name) const {
    return (*pimpl)[c_name];
}

vector<string> CoreManager::getPipelineNames() {
    return pimpl->getPipelineNames();
}

map<string, FunctionUtils> CoreManager::getFunctionUtils() {
    return pimpl->getFunctionUtils();
}

}  // namespace fase
