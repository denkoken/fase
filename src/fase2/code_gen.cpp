
#include "common.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.h"
#include "debug_macros.h"
#include "manager.h"

namespace fase {

using std::string, std::vector, std::map, std::type_index;
using size_t = std::size_t;

class ENDL {};
class UIL {};
class DIL {};

static constexpr ENDL endl;
static constexpr UIL uil;
static constexpr DIL dil;

class BrokenPipeError : public std::runtime_error {
public:
    BrokenPipeError(const string& m) : std::runtime_error(m) {}

    BrokenPipeError(BrokenPipeError&) = default;
    BrokenPipeError(const BrokenPipeError&) = default;
    BrokenPipeError(BrokenPipeError&&) = default;
    BrokenPipeError& operator=(BrokenPipeError&) = default;
    BrokenPipeError& operator=(const BrokenPipeError&) = default;
    BrokenPipeError& operator=(BrokenPipeError&&) = default;

    virtual ~BrokenPipeError() = default;
};

class MyStream {
public:
    MyStream(const string& indent_) : indent(indent_) {}

    template <typename T>
    MyStream& operator<<(T&& v) {
        if constexpr (std::is_same_v<ENDL, std::decay_t<T>>) {
            newLine();
        } else if constexpr (std::is_same_v<UIL, std::decay_t<T>>) {
            upIndentLevel();
        } else if constexpr (std::is_same_v<DIL, std::decay_t<T>>) {
            downIndentLevel();
        } else {
            ss << std::forward<T>(v);
        }
        return *this;
    }

    std::string str() {
        return ss.str();
    }

    void newLine() {
        ss << std::endl;
        for (int i = 0; i < indent_level; i++) {
            ss << indent;
        }
    }

    void upIndentLevel() {
        indent_level++;
    }
    void downIndentLevel() {
        indent_level--;
        assert(indent_level >= 0);
    }

private:
    const std::string indent;
    std::stringstream ss;
    int indent_level = 0;
};

class TSCMapW {
public:
    TSCMapW(const TSCMap& tsc_map) : map(tsc_map) {}

    virtual ~TSCMapW() = default;

    auto& at(const std::type_index& k FASE_COMMA_DEBUG_LOC(loc)) const {
        if (!map.count(k)) {
            throw BrokenPipeError((HEAD + type_name(k) + TAIL
#ifdef FASE_IS_DEBUG_SOURCE_LOCATION_ON
                                   + "\n(thrown by " + loc.file_name() + ":" +
                                   std::to_string(loc.line()) + ")"
#endif
                                   )
                                          .c_str());
        }
        return map.at(k);
    }

    auto count(const std::type_index& k) const {
        return map.count(k);
    }

private:
    const TSCMap& map;
    const std::string HEAD = "Unset RegisterTextIO of user-defined type : ";
    const std::string TAIL =
            ".\nSet RegisterTextIO with FaseRegisterTextIO() macro.";
};

namespace {

bool isFunctionObject(const FunctionUtils& utils) {
    return utils.type != FOGtype::Pure && utils.type != FOGtype::Special;
}

bool isConstructableObject(const FunctionUtils& utils) {
    return utils.type == FOGtype::Lambda ||
           utils.type == FOGtype::IndependingClass;
}

std::tuple<string, size_t> GetSrcName(const string& dst_n_name,
                                      const size_t idx,
                                      const vector<Link>& links) {
    for (auto& link : links) {
        if (link.dst_node == dst_n_name && link.dst_arg == idx) {
            return {link.src_node, link.src_arg};
        }
    }
    return {"", 0};
}

std::string genVarName(const std::string& node_name, const size_t arg_idx) {
    std::stringstream var_name_ss;
    var_name_ss << node_name << "_" << arg_idx;
    return var_name_ss.str();
}

std::string toEnumValueName(const std::string& n_name, const string& p_name) {
    return n_name + "_OF_" + p_name;
}

std::string genVarDeclaration(const std::string& type_repr,
                              const std::string& val_repr,
                              const std::string& name,
                              const std::string& footer = ";",
                              const bool is_clrvalue = false) {
    // Add declaration code (Remove reference for declaration)
    std::stringstream ss;
    if (is_clrvalue) {
        ss << "const " << type_repr << "& " << name;
    } else {
        ss << type_repr << " " << name;
    }
    if (!val_repr.empty()) {
        // Add variable initialization
        ss << " = " << val_repr;
    }
    ss << footer;
    return ss.str();
}

template <typename T>
T PopFront(vector<vector<T>>& set_array) {
    if (set_array.empty()) {
        return T();
    }

    auto dst = *std::begin(set_array[0]);
    set_array[0].erase(std::begin(set_array[0]));

    if (set_array[0].empty()) {
        set_array.erase(std::begin(set_array));
    }

    return dst;
}

string getValStr(const Variable& v, const TSCMapW& tsc_map) {
    if (!tsc_map.count(v.getType()) || !v) {
        return "";
    }
    return tsc_map.at(v.getType()).def_maker(v);
}

/// return [name, is_nessessary_of_genVarDeclaration]
std::tuple<std::string, bool>
searchArgName(const PipelineAPI& pipe_api, const string& node_name,
              const size_t arg_idx, const std::string& arg_repr,
              const map<string, FunctionUtils>& functions) {
    const Node& node = pipe_api.getNodes().at(node_name);
    const FunctionUtils& function = functions.at(node.func_name);

    auto [src, s_idx] = GetSrcName(node_name, arg_idx, pipe_api.getLinks());

    if (src.empty() && function.is_input_args[arg_idx]) {
        // Case 0: Write solid value.
        return {arg_repr, false};
    } else if (src.empty()) {
        // Case 1: Create default argument
        if (function.arg_names[arg_idx] == kReturnValueID) {
            return {kReturnValueID + node_name + "_ret", false};
        }
        return {genVarName(node_name, arg_idx), true};
    } else {
        // Case 2: Use output variable
        if (src == InputNodeName()) {
            const FunctionUtils& in_f = functions.at(kInputFuncName);
            return {in_f.arg_names.at(s_idx), false};
        } else {
            return {std::get<0>(searchArgName(pipe_api, src, s_idx, arg_repr,
                                              functions)),
                    false};
        }
    }
}

vector<string> genLocalVariableDef(MyStream& native_code,
                                   const PipelineAPI& pipe_api,
                                   const string& node_name,
                                   const TSCMapW& tsc_map) {
    const Node& node = pipe_api.getNodes().at(node_name);
    const map<string, FunctionUtils> functions = pipe_api.getFunctionUtils();
    const FunctionUtils& function = functions.at(node.func_name);

    // Argument representations
    const vector<string>& arg_type_reprs = [&] {
        vector<string> dst;
        for (auto& type : function.arg_types) {
            dst.emplace_back(tsc_map.at(type).name);
        }
        return dst;
    }();

    vector<string> arg_reprs;
    for (const auto& v : node.args) {
        arg_reprs.emplace_back(getValStr(v, tsc_map));
    }

    const size_t n_args = node.args.size();
    assert(arg_reprs.size() == n_args);
    vector<string> var_names;
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        auto [var_name, dec_f] = searchArgName(pipe_api, node_name, arg_idx,
                                               arg_reprs[arg_idx], functions);
        if (dec_f) {
            // Add declaration code
            native_code << genVarDeclaration(arg_type_reprs[arg_idx],
                                             arg_reprs[arg_idx], var_name)
                        << endl;
        }
        var_names.push_back(var_name);
    }
    return var_names;
}

void genFuncDeclaration(MyStream& code_stream, const string& func_name,
                        const map<string, Node>& nodes,
                        const map<string, FunctionUtils> functions,
                        const TSCMapW& tsc_map) {
    const Node& in_n = nodes.at(InputNodeName());
    // const Node& out_n = nodes.at(OutputNodeName());

    const FunctionUtils& in_f = functions.at(kInputFuncName);
    const FunctionUtils& out_f = functions.at(kOutputFuncName);

    code_stream << "void " << func_name << "(";
    // Input Arguments
    if (!nodes.at(InputNodeName()).args.empty()) {
        const vector<string>& names = in_f.arg_names;

        size_t n_args = in_n.args.size();

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            code_stream << genVarDeclaration(
                    tsc_map.at(in_f.arg_types[arg_idx]).name, "",
                    names[arg_idx], "", true);
            if (arg_idx != n_args - 1) {
                code_stream << ", ";
            }
        }
    }

    // Output Arguments
    if (!out_f.arg_names.empty()) {
        size_t n_args = out_f.arg_names.size();

        if (!nodes.at(InputNodeName()).args.empty()) {
            code_stream << ", ";
        }

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            code_stream << tsc_map.at(out_f.arg_types[arg_idx]).name << "& "
                        << out_f.arg_names[arg_idx];
            if (arg_idx != n_args - 1) {
                code_stream << ", ";
            }
        }
    }
    code_stream << ") ";
}

void genFunctionCall(MyStream& ss, const string& func_name,
                     const string& node_name, const string& pipe_name,
                     const FunctionUtils& func_util, vector<string> var_names) {
    if (var_names.back().find(kReturnValueID) != std::string::npos) {
        if (var_names.back().find(node_name + "_ret") != std::string::npos) {
            ss << "auto ";
        }
        ss << replace(var_names.back(), kReturnValueID, "") << " = ";
        var_names.pop_back();
    }
    if (isFunctionObject(func_util)) {
        ss << func_name << "s[" << toEnumValueName(node_name, pipe_name) << "]";
    } else {
        ss << func_name;
    }
    ss << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << replace(var_names[i], kReturnValueID, "");
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << endl;
}

void genOutputAssignment(MyStream& native_code, const PipelineAPI& pipe_api,
                         const map<string, FunctionUtils>& functions,
                         const vector<Link>& links) {
    const Node& out_n = pipe_api.getNodes().at(OutputNodeName());
    const FunctionUtils& out_f = functions.at(out_n.func_name);

    if (!out_f.arg_names.empty()) {
        size_t n_args = out_f.arg_names.size();
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto [src, s_idx] = GetSrcName(OutputNodeName(), arg_idx, links);
            // auto& link = out_n.links[arg_idx];
            if (src.empty()) {
                continue;
            }
            native_code << endl;
            auto [val_name, _] = searchArgName(pipe_api, OutputNodeName(),
                                               arg_idx, "", functions);
            native_code << out_f.arg_names[arg_idx] << " = " << val_name << ";";
        }
    }
}

bool genFunctionCode(MyStream& native_code, const string& p_name,
                     const CoreManager& cm, const TSCMapW& tsc_map,
                     const string& entry_name) {
    auto functions = cm.getFunctionUtils(p_name);
    auto& nodes = cm[p_name].getNodes();
    auto& links = cm[p_name].getLinks();
    // Stack for finding runnable node
    auto node_order = GetRunOrder(nodes, links);

    string pipe_func_name = entry_name;
    if (pipe_func_name.empty()) {
        pipe_func_name = "Pipeline";
    }

    genFuncDeclaration(native_code, pipe_func_name, nodes, functions, tsc_map);
    native_code << "{" << uil << endl;

    while (true) {
        // Find runnable node
        const string n_name = PopFront(node_order);
        if (n_name.empty()) {
            break;
        }

        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            continue;
        }
        const string& func_name = nodes.at(n_name).func_name;

        // Add comment
        native_code << "// " << func_name << " [" << n_name << "]" << endl;

        vector<string> var_names =
                genLocalVariableDef(native_code, cm[p_name], n_name, tsc_map);

        // Add function call
        genFunctionCall(native_code, func_name, n_name, p_name,
                        functions.at(func_name), var_names);
    }
    assert(node_order.empty());

    genOutputAssignment(native_code, cm[p_name], functions,
                        cm[p_name].getLinks());

    native_code << dil << endl << "}";

    return true;
}

struct N_ID {
    string pipe, node;
};

map<string, vector<N_ID>> GetNonPureNodeMap(const vector<string>& pipes,
                                            const CoreManager& cm) {
    map<string, vector<N_ID>> dst;
    for (auto& p_name : pipes) {
        auto f_utils = cm.getFunctionUtils(p_name);
        for (auto& [n_name, node] : cm[p_name].getNodes()) {
            if (isFunctionObject(f_utils.at(node.func_name))) {
                dst[node.func_name].push_back({p_name, n_name});
            }
        }
    }
    return dst;
}

void GenFunctionArrays(MyStream& native_code,
                       const map<string, vector<N_ID>>& non_pure_node_map,
                       const map<string, FunctionUtils>& f_utils) {
    for (auto& [f_name, vs] : non_pure_node_map) {
        if (isConstructableObject(f_utils.at(f_name))) {
            native_code << "std::array<"
                        << type_name(*f_utils.at(f_name).callable_type);
            native_code << ", " << vs.size() << "> " << f_name + "s = {" << uil
                        << uil;
            for (size_t i = 0; i < vs.size(); i++) {
                native_code << endl << f_utils.at(f_name).repr << ",";
            }
            native_code << dil << dil << endl << "};" << endl;
        } else {
            native_code << "std::array<std::function<void"
                        << f_utils.at(f_name).arg_types_repr;
            native_code << ">, " << vs.size() << "> " << f_name + "s;" << endl;
        }

        for (size_t i = 0; i < vs.size(); i++) {
            native_code << "constexpr static int "
                        << toEnumValueName(vs[i].node, vs[i].pipe) << " = " << i
                        << ";" << endl;
        }
    }
}

void GenSetters(MyStream& native_code,
                const map<string, vector<N_ID>>& non_pure_node_map,
                const map<string, FunctionUtils>& f_utils) {
    for (auto& [f_name, vs] : non_pure_node_map) {
        if (isConstructableObject(f_utils.at(f_name))) {
            continue;
        }
        native_code << "template <class Callable>" << endl;
        native_code << "void set_" << f_name << "(Callable&& " << f_name
                    << ") {" << uil << endl;
        native_code << "for (int i = 0; i < " << vs.size() << "; i++) {" << uil
                    << endl;
        native_code << f_name << "s[i] = " << f_name << ";";
        native_code << dil << endl << "}";
        native_code << dil << endl << "}" << endl << endl;
    }
}

} // namespace

string GenNativeCode(const string& p_name, const CoreManager& cm,
                     const TSCMap& tsc_map, const string& entry_name,
                     const string& indent) {
    MyStream native_code{indent};
    TSCMapW tsc_map_wraped(tsc_map);
    try {

        // TODO change local function array member to local members.

        native_code << "class " << entry_name << " {" << endl;
        native_code << "private:" << uil << endl;

        auto sub_pipes = cm.getDependingTree().getDependenceLayer(p_name);
        vector<string> pipes = {p_name};
        for (auto& ps : sub_pipes) {
            Extend(ps, &pipes);
        }
        auto non_pure_node_map = GetNonPureNodeMap(pipes, cm);
        GenFunctionArrays(native_code, non_pure_node_map,
                          cm.getFunctionUtils(p_name));

        native_code << endl;

        // write sub pipeline function Definitions.
        for (auto iter = sub_pipes.rbegin(); iter != sub_pipes.rend(); iter++) {
            for (auto& sub_pipe_name : *iter) {
                genFunctionCode(native_code, sub_pipe_name, cm, tsc_map_wraped,
                                sub_pipe_name);
                native_code << endl << endl;
            }
        }
        native_code << dil << endl << "public:" << uil << endl;

        GenSetters(native_code, non_pure_node_map, cm.getFunctionUtils(p_name));

        // write main pipeline function Definitions.
        genFunctionCode(native_code, p_name, cm, tsc_map_wraped, "operator()");

        native_code << dil << endl << "};";

        return native_code.str();

    } catch (BrokenPipeError& e) {
        return e.what();
    } catch (std::exception& e) {
        std::cerr << "genNativeCode() Error : " << e.what() << std::endl;
        return native_code.str() + "\n" + string(80, '=') +
               "\nFailed to generate code.";
    } catch (...) {
        std::cerr << "genNativeCode() Error : something went wrong."
                  << std::endl;
        return native_code.str() + "\n" + string(80, '=') +
               "\nFailed to generate code.";
    }
}

} // namespace fase
