
#include "manager.h"

#include <iostream>
#include <map>
#include <string>

#include "constants.h"
#include "core.h"
#include "utils.h"

namespace fase {

using std::deque;
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
            " ",  "__", ".", ",", "@", ":", "&", "%", "+",  "-",
            "\\", "^",  "~", "=", "(", ")", "#", "$", "\"", "!",
            "<",  ">",  "?", "{", "}", "[", "]", "`",
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

vector<std::type_index> getTypes(const deque<Variable>& vars) {
    vector<std::type_index> a;
    for (auto& var : vars) {
        a.emplace_back(var.getType());
    }
    return a;
}

void CallCore(Core* pcore, const string& c_name, deque<Variable>& vs,
              Report* preport) {
    size_t i_size = pcore->getNodes().at(InputNodeName()).args.size();
    size_t o_size = pcore->getNodes().at(OutputNodeName()).args.size();
    if (vs.size() != i_size + o_size) {
        throw std::logic_error("Invalid size of variables at Binded Pipe.");
    }
    deque<Variable> inputs, outputs;
    RefCopy(vs.begin(), vs.begin() + long(i_size), &inputs);
    RefCopy(vs.begin() + long(i_size), vs.end(), &outputs);

    pcore->supposeInput(inputs);
    pcore->supposeOutput(outputs);
    if (!pcore->run(preport)) {
        throw(std::runtime_error(c_name + " is failed!"));
    }
}

} // namespace

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

    LinkNodeError smartLink(const std::string&, size_t, const std::string&,
                            size_t) override {
        return LinkNodeError::Another;
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

    bool call(deque<Variable>&) override {
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
    map<string, FunctionUtils> getFunctionUtils() const override {
        return {};
    }

private:
    std::map<std::string, Node> dum_n;
    std::vector<Link> dum_l;
};

// ============================== CoreManager ==================================

struct Function {
    UnivFunc func;
    deque<Variable> default_args;
    FunctionUtils utils;
};

class CoreManager::Impl {
public:
    Impl() = default;
    Impl(const Impl&) = default;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = default;
    Impl& operator=(Impl&&) = delete;
    ~Impl() = default;

    bool addUnivFunc(const UnivFunc& func, const std::string& name,
                     std::deque<Variable>&& default_args,
                     FunctionUtils&& utils);

    PipelineAPI& operator[](const string& c_name);
    const PipelineAPI& operator[](const string& c_name) const;

    void setFocusedPipeline(const std::string& p_name) {
        focused_pipeline_name = p_name;
    }
    std::string getFocusedPipeline() const {
        return focused_pipeline_name;
    }

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

    string focused_pipeline_name;

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
        if (n_name == InputNodeName()) {
            inputs[idx] = var.ref();
            core.supposeInput(inputs);
            cm_ref.get().updateBindedPipes(myname());
            return true;
        } else if (n_name == OutputNodeName()) {
            outputs[idx] = var.ref();
            core.supposeOutput(outputs);
            cm_ref.get().updateBindedPipes(myname());
            return true;
        } else if (core.setArgument(n_name, idx, var)) {
            return true;
        } else {
            return false;
        }
    }
    bool setPriority(const string& n_name, int priority) override {
        return core.setPriority(n_name, priority);
    }

    bool allocateFunc(const string& f_name, const string& n_name) override {
        if (!core.getNodes().count(n_name)) {
            return false;
        }
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

    LinkNodeError smartLink(const string& src_node, size_t src_arg,
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

    bool call(deque<Variable>& args) override;
    bool run(Report* preport = nullptr) override;

    const map<string, Node>& getNodes() const noexcept override {
        return core.getNodes();
    }
    const vector<Link>& getLinks() const noexcept override {
        return core.getLinks();
    }
    map<string, FunctionUtils> getFunctionUtils() const override {
        return cm_ref.get().getFunctionUtils(myname());
    }

    Core core;
    std::reference_wrapper<Impl> cm_ref;

    deque<Variable> inputs;
    deque<Variable> outputs;
    vector<string> input_var_names;
    vector<string> output_var_names;

    const string& myname() const {
        for (auto& [c_name, wrapeds] : cm_ref.get().wrapeds) {
            if (&wrapeds == this) return c_name;
        }
        throw std::logic_error("myname is not found!");
    }
};

// ======================== WrapedCore Member Functions ========================

LinkNodeError CoreManager::Impl::WrapedCore::smartLink(const string& src_node,
                                                       size_t src_arg,
                                                       const string& dst_node,
                                                       size_t dst_arg) {
    LinkNodeError err = core.linkNode(src_node, src_arg, dst_node, dst_arg);
    if (err != LinkNodeError::InvalidType) return err;

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
    return err;
}

bool CoreManager::Impl::WrapedCore::call(deque<Variable>& args) {
    if (args.size() != inputs.size() + outputs.size()) {
        return false;
    }
    for (size_t i = 0; i < inputs.size(); i++) {
        if (args[i].getType() != inputs[i].getType()) {
            return false;
        }
    }
    for (size_t i = 0; i < outputs.size(); i++) {
        if (args[i + inputs.size()].getType() != outputs[i].getType()) {
            return false;
        }
    }

    CallCore(&core, myname(), args, nullptr);
    return true;
}

bool CoreManager::Impl::WrapedCore::run(Report* preport) {
    return core.run(preport);
}

// ========================== Impl Member Functions ============================

bool CoreManager::Impl::addFunction(const string& func, const string& core) {
    deque<Variable> vs;
    RefCopy(functions[func].default_args, &vs);
    return wrapeds.at(core).core.addUnivFunc(functions[func].func, func,
                                             std::move(vs));
}

bool CoreManager::Impl::addUnivFunc(const UnivFunc& func, const string& f_name,
                                    deque<Variable>&& default_args,
                                    FunctionUtils&& utils) {
    if (wrapeds.count(f_name)) return false;

    functions[f_name] = {
            func,
            std::move(default_args),
            std::move(utils),
    };
    for (auto& [c_name, _] : wrapeds) {
        if (!addFunction(f_name, c_name)) return false;
    }
    return true;
}

bool CoreManager::Impl::newPipeline(const string& c_name) {
    if (wrapeds.count(c_name) || functions.count(c_name)) return false;

    addUnivFunc(
            {}, c_name, {},
            {{}, {}, {}, FOGtype::OtherPipe, "", {}, "", "Another pipeline"});

    wrapeds.emplace(c_name, *this); // create new WrapedCore.
    for (auto& [f_name, func] : functions) {
        if (c_name != f_name) {
            addFunction(f_name, c_name);
        }
    }
    updateBindedPipes(c_name);
    return true;
}

bool CoreManager::Impl::updateBindedPipes(const string& c_name) {
    Function& func = functions[c_name];
    auto& core = wrapeds.at(c_name).core;

    // Update Function::func (UnivFunc)
    func.func = [&core, c_name](deque<Variable>& vs, Report* preport) {
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
    Extend(wrapeds.at(c_name).output_var_names, &func.utils.arg_names);

    // Update Function::utils::arg_types and is_input_args
    func.utils.arg_types.clear();
    func.utils.is_input_args.clear();
    for (auto& var : wrapeds.at(c_name).inputs) {
        func.utils.arg_types.emplace_back(var.getType());
        func.utils.is_input_args.emplace_back(true);
    }
    for (auto& var : wrapeds.at(c_name).outputs) {
        func.utils.arg_types.emplace_back(var.getType());
        func.utils.is_input_args.emplace_back(false);
    }

    // Add updated function to other pipelines.
    for (auto& [other_c_name, _] : wrapeds) {
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
    if (!wrapeds.count(c_name) &&
        (!CheckGoodVarName(c_name) || !newPipeline(c_name))) {
        return dum;
    }
    return wrapeds.at(c_name);
}

const PipelineAPI& CoreManager::Impl::operator[](const string& c_name) const {
    if (!wrapeds.count(c_name)) {
        return dum;
    }
    return wrapeds.at(c_name);
}

bool ExportedPipe::operator()(std::deque<Variable>& vs) {
    if (!reseter) {
        return false;
    }
    if (vs.size() != types.size()) {
        return false;
    }
    for (size_t i = 0; i < vs.size(); i++) {
        if (vs[i].getType() != types[i]) {
            return false;
        }
    }

    CallCore(&core, "ExportedPipe", vs, nullptr);
    return true;
}

ExportedPipe CoreManager::Impl::exportPipe(const std::string& e_c_name) const {
    if (!wrapeds.count(e_c_name)) {
        return {{}, {}, {}};
    }
    auto d_layer = dependence_tree.getDependenceLayer(e_c_name);
    d_layer.emplace(d_layer.begin(), vector<string>{e_c_name});

    map<string, Core> cores;
    map<string, deque<Variable>> default_args_map;
    for (auto& cs : d_layer) {
        for (auto& c_name : cs) {
            cores.emplace(c_name, wrapeds.at(c_name).core);
            deque<Variable> default_args = wrapeds.at(c_name).inputs;
            Extend(wrapeds.at(c_name).outputs, &default_args);
            default_args_map.emplace(c_name, std::move(default_args));
        }
    }

    for (auto it = d_layer.rbegin() + 1; it != d_layer.rend(); it++) {
        for (auto& c_name : *it) {
            for (auto d_c_name : dependence_tree.getDependings(c_name)) {
                struct {
                    Core core;
                    const string c_name;
                    void operator()(deque<Variable>& vs, Report*) {
                        CallCore(&core, c_name, vs, nullptr);
                    }
                } f = {cores.at(d_c_name), d_c_name};
                cores.at(c_name).addUnivFunc(
                        std::move(f), d_c_name,
                        deque<Variable>{default_args_map.at(d_c_name)});
            }
        }
    }
    vector<std::type_index> types;
    for (auto& v : wrapeds.at(e_c_name).inputs) {
        types.emplace_back(v.getType());
    }
    for (auto& v : wrapeds.at(e_c_name).outputs) {
        types.emplace_back(v.getType());
    }

    Core core = std::move(cores.at(e_c_name));
    Core core2 = core;

    return ExportedPipe{std::move(core),
                        [default_core = std::move(core2)](Core* pcore) {
                            *pcore = default_core;
                        },
                        std::move(types)};
}

vector<string> CoreManager::Impl::getPipelineNames() const {
    vector<string> dst;
    for (auto& [c_name, _] : wrapeds) {
        dst.emplace_back(c_name);
    }
    return dst;
}

map<string, FunctionUtils>
CoreManager::Impl::getFunctionUtils(const string& p_name) const {
    if (!wrapeds.count(p_name)) {
        return {};
    }
    map<string, FunctionUtils> dst;
    dst[""];
    auto& i_names = wrapeds.at(p_name).input_var_names;
    dst[kInputFuncName] = {i_names,
                           getTypes(wrapeds.at(p_name).inputs),
                           vector<bool>(i_names.size(), false),
                           FOGtype::Special,
                           "",
                           {},
                           "",
                           ""};
    auto& o_names = wrapeds.at(p_name).output_var_names;
    dst[kOutputFuncName] = {o_names,
                            getTypes(wrapeds.at(p_name).outputs),
                            vector<bool>(o_names.size(), true),
                            FOGtype::Special,
                            "",
                            {},
                            "",
                            ""};

    for (auto& [f_name, func] : functions) {
        if (f_name != p_name) {
            dst[f_name] = func.utils;
        }
    }
    return dst;
}

// ============================== Pimpl Pattern ================================

CoreManager::CoreManager() : pimpl(std::make_unique<Impl>()) {}
CoreManager::CoreManager(const CoreManager& a)
    : pimpl(std::make_unique<Impl>(*a.pimpl)) {}
CoreManager::CoreManager(CoreManager& a)
    : pimpl(std::make_unique<Impl>(*a.pimpl)) {}
CoreManager::CoreManager(CoreManager&&) = default;
CoreManager& CoreManager::operator=(const CoreManager& a) {
    *pimpl = *a.pimpl;
    return *this;
}
CoreManager& CoreManager::operator=(CoreManager& a) {
    *pimpl = *a.pimpl;
    return *this;
}
CoreManager& CoreManager::operator=(CoreManager&&) = default;
CoreManager::~CoreManager() = default;

bool CoreManager::addUnivFunc(const UnivFunc& func, const string& c_name,
                              deque<Variable>&& default_args,
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

void CoreManager::setFocusedPipeline(const string& p_name) {
    return pimpl->setFocusedPipeline(p_name);
}
string CoreManager::getFocusedPipeline() const {
    return pimpl->getFocusedPipeline();
}

ExportedPipe CoreManager::exportPipe(const std::string& name) const {
    return pimpl->exportPipe(name);
}

vector<string> CoreManager::getPipelineNames() const {
    return pimpl->getPipelineNames();
}

map<string, FunctionUtils>
CoreManager::getFunctionUtils(const string& p_name) const {
    return pimpl->getFunctionUtils(p_name);
}

const DependenceTree& CoreManager::getDependingTree() const {
    return pimpl->getDependingTree();
}

} // namespace fase
