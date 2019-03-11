
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

namespace {

bool CheckGoodVarName(const string& name) {
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

}  // namespace

class FaildDummy : public PipelineAPI {
    bool newNode(const std::string&) override {
        return false;
    }
    bool renameNode(const std::string&, const std::string&) override {
        return false;
    }
    bool delNode(const std::string&) override {
        return false;
    }

    bool setArgument(const std::string&, size_t, Variable&) override {
        return false;
    }
    bool setPriority(const std::string&, int) override {
        return false;
    }

    bool allocateFunc(const std::string&, const std::string&) override {
        return false;
    }

    bool smartLink(const std::string&, size_t, const std::string&,
                   size_t) override {
        return false;
    }
    bool unlinkNode(const std::string&, size_t) override {
        return false;
    }

    bool supposeInput(const std::vector<std::string>&) override {
        return false;
    }
    bool supposeOutput(const std::vector<std::string>&) override {
        return false;
    }

    bool run(Report* = nullptr) override {
        return false;
    }

    const std::map<std::string, Node>& getNodes() const noexcept override {
        return dum_n;
    }

    const std::vector<Link>& getLinks() const noexcept override {
        return dum_l;
    }

private:
    std::map<std::string, Node> dum_n;
    std::vector<Link> dum_l;
};

void CallCore(Core* pcore, const string& c_name, vector<Variable>& vs,
              Report* preport) {
    size_t i_size = pcore->getNodes().at(InputNodeName()).args.size();
    size_t o_size = pcore->getNodes().at(OutputNodeName()).args.size();
    if (vs.size() != i_size + o_size) {
        throw std::logic_error("Invalid size of variables at Binded Pipe.");
    }
    vector<Variable> inputs, outputs;
    RefCopy(vs.begin(), vs.begin() + long(i_size), &inputs);
    RefCopy(vs.begin() + long(i_size), vs.end(), &outputs);

    pcore->supposeInput(inputs);
    pcore->supposeOutput(outputs);
    if (!pcore->run(preport)) {
        throw(std::runtime_error(c_name + " is failed!"));
    }
}

// ============================== CoreManager ==================================

struct Function {
    UnivFunc func;
    vector<Variable> default_args;
    FunctionUtils utils;
};

class CoreManager::Impl {
public:
    Impl() = default;
    ~Impl() = default;
    bool addUnivFunc(const UnivFunc& func, const std::string& name,
                     std::vector<Variable>&& default_args,
                     FunctionUtils&& utils);

    PipelineAPI& operator[](const string& c_name);
    const PipelineAPI& operator[](const string& c_name) const;

    ExportedPipe exportPipe(const std::string& name) const;

    vector<string> getPipelineNames() const;
    map<string, FunctionUtils> getFunctionUtils(const string& p_name) const;
    const DependenceTree& getDependingTree() const {
        return dependence_tree;
    }

private:
    class WrapedCore;

    map<string, WrapedCore> wrapeds;
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
        if (!CheckGoodVarName(n_name)) return false;
        return core.newNode(n_name);
    }

    bool renameNode(const string& old_n_name,
                    const string& new_n_name) override {
        if (!CheckGoodVarName(new_n_name)) return false;
        return core.renameNode(old_n_name, new_n_name);
    }
    bool delNode(const string& n_name) override {
        auto& d_tree = cm_ref.get().dependence_tree;
        d_tree.del(myname(), core.getNodes().at(n_name).func_name);
        return core.delNode(n_name);
    }

    bool setArgument(const string& n_name, size_t idx, Variable& var) override {
        if (!core.setArgument(n_name, idx, var)) {
            return false;
        }
        if (n_name == InputNodeName()) {
            inputs[idx] = var.ref();
            core.supposeInput(inputs);
            cm_ref.get().updateBindedPipes(myname());
        } else if (n_name == OutputNodeName()) {
            outputs[idx] = var.ref();
            core.supposeOutput(outputs);
            cm_ref.get().updateBindedPipes(myname());
        }
        return true;
    }
    bool setPriority(const string& n_name, int priority) override {
        return core.setPriority(n_name, priority);
    }

    bool allocateFunc(const string& f_name, const string& n_name) override {
        auto& d_tree = cm_ref.get().dependence_tree;
        if (cm_ref.get().wrapeds.count(core.getNodes().at(n_name).func_name)) {
            d_tree.del(myname(), core.getNodes().at(n_name).func_name);
        }
        if (cm_ref.get().wrapeds.count(f_name)) {
            if (d_tree.add(myname(), f_name)) {
                return core.allocateFunc(f_name, n_name);
            } else {
                return false;
            }
        }
        return core.allocateFunc(f_name, n_name);
    }

    bool smartLink(const string& src_node, size_t src_arg,
                   const string& dst_node, size_t dst_arg) override;
    bool unlinkNode(const string& dst_node, size_t dst_arg) override {
        return core.unlinkNode(dst_node, dst_arg);
    }

    bool supposeInput(const std::vector<std::string>& arg_names) override {
        for (auto& name : arg_names) {
            if (!CheckGoodVarName(name)) {
                return false;
            }
        }
        if (CheckRepetition(arg_names, output_var_names)) {
            return false;
        }
        inputs.resize(arg_names.size());
        if (core.supposeInput(inputs)) {
            input_var_names = arg_names;
            cm_ref.get().updateBindedPipes(myname());
            return true;
        }
        return false;
    }

    bool supposeOutput(const std::vector<std::string>& arg_names) override {
        for (auto& name : arg_names) {
            if (!CheckGoodVarName(name)) {
                return false;
            }
        }
        if (CheckRepetition(arg_names, input_var_names)) {
            return false;
        }
        outputs.resize(arg_names.size());
        if (core.supposeOutput(outputs)) {
            output_var_names = arg_names;
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
        for (auto& [c_name, wrapeds] : cm_ref.get().wrapeds) {
            if (&wrapeds == this) return c_name;
        }
        throw std::logic_error("myname is not found!");
    }
};

// ======================== WrapedCore Member Functions ========================

bool CoreManager::Impl::WrapedCore::smartLink(const string& src_node,
                                              size_t src_arg,
                                              const string& dst_node,
                                              size_t dst_arg) {
    if (!core.getNodes().count(src_node)) return false;
    if (!core.getNodes().count(dst_node)) return false;
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
    return wrapeds.at(core).core.addUnivFunc(functions[func].func, func,
                                             std::move(vs));
}

bool CoreManager::Impl::addUnivFunc(const UnivFunc& func, const string& f_name,
                                    vector<Variable>&& default_args,
                                    FunctionUtils&& utils) {
    if (wrapeds.count(f_name)) return false;

    functions[f_name] = {
            func,
            std::move(default_args),
            std::move(utils),
    };
    for (auto& [c_name, wrapeds] : wrapeds) {
        if (!addFunction(f_name, c_name)) return false;
    }
    return true;
}

bool CoreManager::Impl::newPipeline(const string& c_name) {
    if (wrapeds.count(c_name) || functions.count(c_name)) return false;

    addUnivFunc(UnivFunc{}, c_name, {}, {{}, {}, {}, true, "Another pipeline"});

    wrapeds.emplace(c_name, *this);  // create new WrapedCore.
    for (auto& [f_name, func] : functions) {
        if (c_name != f_name) {
            addFunction(f_name, c_name);
        }
    }
    return true;
}

bool CoreManager::Impl::updateBindedPipes(const string& c_name) {
    Function& func = functions[c_name];
    auto& core = wrapeds.at(c_name).core;

    // Update Function::func (UnivFunc)
    func.func = [&core, c_name](vector<Variable>& vs, Report* preport) {
        CallCore(&core, c_name, vs, preport);
    };

    // Update Function::default_args.
    size_t i_size = core.getNodes().at(InputNodeName()).args.size();
    RefCopy(wrapeds.at(c_name).inputs, &func.default_args);
    func.default_args.resize(i_size + wrapeds.at(c_name).outputs.size());
    for (size_t i = 0; i < wrapeds.at(c_name).outputs.size(); i++) {
        func.default_args[i + i_size] = wrapeds.at(c_name).outputs[i].ref();
    }

    // Update Function::utils::arg_names.
    func.utils.arg_names = wrapeds.at(c_name).input_var_names;
    Extend(vector<string>{wrapeds.at(c_name).output_var_names},
           &func.utils.arg_names);

    // Update Function::utils::arg_types and is_input_args
    func.utils.arg_types.clear();
    func.utils.is_input_args.clear();
    for (auto& var : wrapeds.at(c_name).inputs) {
        func.utils.arg_types.emplace_back(var.getType());
        func.utils.is_input_args.emplace_back(false);
    }
    for (auto& var : wrapeds.at(c_name).outputs) {
        func.utils.arg_types.emplace_back(var.getType());
        func.utils.is_input_args.emplace_back(true);
    }

    // Add updated function to other pipelines.
    for (auto& [other_c_name, wrapeds] : wrapeds) {
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
    if (!wrapeds.count(c_name)) {
        if (!CheckGoodVarName(c_name)) {
            return dum;
        } else if (!newPipeline(c_name)) {
            return dum;
        }
    }
    return wrapeds.at(c_name);
}

const PipelineAPI& CoreManager::Impl::operator[](const string& c_name) const {
    if (!wrapeds.count(c_name)) {
        return dum;
    }
    return wrapeds.at(c_name);
}

void ExportedPipe::operator()(std::vector<Variable>& vs) {
    CallCore(&core, "ExportedPipe", vs, nullptr);
}

ExportedPipe CoreManager::Impl::exportPipe(const std::string& e_c_name) const {
    auto d_layer = dependence_tree.getDependenceLayer(e_c_name);
    d_layer.emplace(d_layer.begin(), vector<string>{e_c_name});

    map<string, Core> cores;
    map<string, vector<Variable>> default_args_map;
    for (auto& cs : d_layer) {
        for (auto& c_name : cs) {
            cores.emplace(c_name, wrapeds.at(c_name).core);
            vector<Variable> default_args = wrapeds.at(c_name).inputs;
            Extend(std::vector<Variable>{wrapeds.at(c_name).outputs},
                   &default_args);
            default_args_map.emplace(c_name, std::move(default_args));
        }
    }

    for (auto it = d_layer.rbegin() + 1; it != d_layer.rend(); it++) {
        for (auto& c_name : *it) {
            for (auto d_c_name : dependence_tree.getDependings(c_name)) {
                struct {
                    Core core;
                    const string c_name;
                    void operator()(vector<Variable>& vs, Report*) {
                        CallCore(&core, c_name, vs, nullptr);
                    }
                } f = {cores.at(d_c_name), d_c_name};
                cores.at(c_name).addUnivFunc(
                        std::move(f), d_c_name,
                        vector<Variable>{default_args_map.at(d_c_name)});
            }
        }
    }

    Core core = std::move(cores.at(e_c_name));

    return ExportedPipe{Core{core},
                        [default_core = std::move(core)](Core* pcore) {
                            *pcore = default_core;
                        }};
}

vector<string> CoreManager::Impl::getPipelineNames() const {
    vector<string> dst;
    for (auto& [c_name, wrapeds] : wrapeds) {
        dst.emplace_back(c_name);
    }
    return dst;
}

map<string, FunctionUtils> CoreManager::Impl::getFunctionUtils(
        const string& p_name) const {
    if (!wrapeds.count(p_name)) {
        return {};
    }
    map<string, FunctionUtils> dst;
    dst[""];
    dst[kInputFuncName] = {
            wrapeds.at(p_name).input_var_names, {}, {}, true, ""};
    for (auto& input : wrapeds.at(p_name).inputs) {
        dst[kInputFuncName].arg_types.emplace_back(input.getType());
        dst[kInputFuncName].is_input_args.emplace_back(false);
    }
    dst[kOutputFuncName] = {
            wrapeds.at(p_name).output_var_names, {}, {}, true, ""};
    for (auto& output : wrapeds.at(p_name).outputs) {
        dst[kOutputFuncName].arg_types.emplace_back(output.getType());
        dst[kOutputFuncName].is_input_args.emplace_back(true);
    }

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
                              FunctionUtils&& utils) {
    return pimpl->addUnivFunc(func, c_name, move(default_args),
                              std::move(utils));
}

PipelineAPI& CoreManager::operator[](const string& c_name) {
    return (*pimpl)[c_name];
}
const PipelineAPI& CoreManager::operator[](const string& c_name) const {
    return std::as_const(*pimpl)[c_name];
}

ExportedPipe CoreManager::exportPipe(const std::string& name) const {
    return pimpl->exportPipe(name);
}

vector<string> CoreManager::getPipelineNames() const {
    return pimpl->getPipelineNames();
}

map<string, FunctionUtils> CoreManager::getFunctionUtils(
        const string& p_name) const {
    return pimpl->getFunctionUtils(p_name);
}

const DependenceTree& CoreManager::getDependingTree() const {
    return pimpl->getDependingTree();
}

}  // namespace fase
