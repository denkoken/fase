
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

using ArgID = std::tuple<string, int>; // node name, idx

class ENDL {};
class UIL {};
class DIL {};

static constexpr ENDL endl;
static constexpr UIL uil;
static constexpr DIL dil;

class BrokenPipeError : public std::runtime_error {
public:
    BrokenPipeError(const string& m) : std::runtime_error(m) {}

    BrokenPipeError(const BrokenPipeError&) = default;
    BrokenPipeError(BrokenPipeError&&) = default;
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
    return utils.type != FOGtype::Pure && utils.type != FOGtype::Special &&
           utils.type != FOGtype::OtherPipe;
}

bool isConstructableObject(const FunctionUtils& utils) {
    return utils.type == FOGtype::Lambda ||
           utils.type == FOGtype::IndependingClass;
}

ArgID SearchRootNodeArg(const PipelineAPI& papi, string n_name, size_t i) {
    auto f = [&n_name, &i, &papi]() {
        for (const Link& l : papi.getLinks()) {
            if (l.dst_node == n_name && l.dst_arg == i) {
                n_name = l.src_node;
                i = l.src_arg;
                return true;
            }
        }
        return false;
    };
    while (1) {
        if (!f()) break;
    }
    return {n_name, i};
}

map<ArgID, string> GenLocalValueName(const PipelineAPI& papi) {
    map<ArgID, string> dst;
    auto f_utils = papi.getFunctionUtils();

    auto f = [&](auto& n_name, auto& node, auto i) {
        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            return f_utils[node.func_name].arg_names[i];
        }
        auto [root_n_name, root_idx] = SearchRootNodeArg(papi, n_name, i);
        string a_name = f_utils[papi.getNodes().at(root_n_name).func_name]
                                .arg_names[root_idx];
        if (root_n_name == InputNodeName()) {
            return a_name;
        }
        if (a_name == kReturnValueID) {
            a_name = "ret";
        }
        std::stringstream var_name_ss;
        var_name_ss << root_n_name << "_" << a_name;
        return var_name_ss.str();
    };

    for (auto& [n_name, node] : papi.getNodes()) {
        for (size_t i = 0; i < node.args.size(); i++) {
            dst[{n_name, i}] = f(n_name, node, i);
        }
    }
    return dst;
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

string getValStr(const Variable& v, const TSCMapW& tsc_map) {
    if (!tsc_map.count(v.getType()) || !v) {
        return "";
    }
    return tsc_map.at(v.getType()).def_maker(v);
}

void genFuncDeclaration(MyStream& code_stream,
                        map<std::string, int>& lv_counter,
                        const map<ArgID, string>& var_names,
                        const string& func_name, const map<string, Node>& nodes,
                        const map<string, FunctionUtils> functions,
                        const TSCMapW& tsc_map) {
    const Node& in_n = nodes.at(InputNodeName());

    const FunctionUtils& in_f = functions.at(kInputFuncName);
    const FunctionUtils& out_f = functions.at(kOutputFuncName);

    code_stream << "void " << func_name << "(";

    size_t n_args = in_n.args.size();

    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        string var_name = var_names.at({InputNodeName(), arg_idx});
        lv_counter[var_name]++;
        code_stream << genVarDeclaration(
                tsc_map.at(in_f.arg_types[arg_idx]).name, "", var_name, "",
                true);
        if (arg_idx != n_args - 1) {
            code_stream << ", ";
        }
    }

    // Output Arguments
    if (!out_f.arg_names.empty()) {
        size_t n_args = out_f.arg_names.size();

        if (!nodes.at(InputNodeName()).args.empty()) {
            code_stream << ", ";
        }

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            string var_name = var_names.at({OutputNodeName(), arg_idx});
            lv_counter[var_name]++;
            code_stream << tsc_map.at(out_f.arg_types[arg_idx]).name << "& "
                        << var_name;
            if (arg_idx != n_args - 1) {
                code_stream << ", ";
            }
        }
    }
    code_stream << ") ";
}

bool genNodeCode(MyStream& native_code, map<std::string, int>& lv_counter,
                 const map<ArgID, string>& var_names, const string& n_name,
                 const string& p_name, const TSCMapW& tsc_map,
                 const map<string, Node>& nodes, const FunctionUtils& func) {
    const string& func_name = nodes.at(n_name).func_name;

    // Add comment
    native_code << "// " << func_name << " [" << n_name << "]" << endl;

    int len_arg = func.arg_names.size() -
                  int(func.arg_names.back() == kReturnValueID);
    for (int i = 0; i < len_arg; i++) {
        string var_name = var_names.at({n_name, i});
        if (!(lv_counter[var_name]++)) {
            string type_str = tsc_map.at(func.arg_types[i]).name;
            string val_str = getValStr(nodes.at(n_name).args[i], tsc_map);
            native_code << genVarDeclaration(type_str, val_str, var_name)
                        << endl;
        }
    }
    if (func.arg_names.back() == kReturnValueID) {
        string var_name = var_names.at({n_name, len_arg});
        if (!(lv_counter[var_name]++)) {
            std::string type_str = tsc_map.at(func.arg_types.back()).name;
            native_code << type_str << " ";
        }
        native_code << var_name << " = ";
    }

    if (isFunctionObject(func)) {
        native_code << toEnumValueName(n_name, p_name);
    } else {
        native_code << func_name;
    }
    native_code << "(";
    for (int i = 0; i < len_arg; i++) {
        native_code << var_names.at({n_name, i});
        if (i != len_arg - 1) {
            native_code << ", ";
        }
    }
    native_code << ");" << endl;
    return true;
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

    map<ArgID, string> var_names = GenLocalValueName(cm[p_name]);
    std::map<std::string, int> lv_counter;

    genFuncDeclaration(native_code, lv_counter, var_names, pipe_func_name,
                       nodes, functions, tsc_map);
    native_code << "{" << uil << endl;

    for (auto& n_name : to1dim(node_order)) {
        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            continue;
        }
        genNodeCode(native_code, lv_counter, var_names, n_name, p_name, tsc_map,
                    nodes, functions.at(nodes.at(n_name).func_name));
    }

    for (size_t i = 0; i < functions.at(kOutputFuncName).arg_names.size();
         i++) {
        native_code
                << endl
                << var_names[{OutputNodeName(), i}] << " = "
                << var_names[SearchRootNodeArg(cm[p_name], OutputNodeName(), i)]
                << ";";
    }

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
        if (f_utils.at(f_name).type == FOGtype::Lambda) {
            native_code << "using " << f_name << "Type = "
                        << type_name(*f_utils.at(f_name).callable_type) << ";"
                        << endl;
            for (size_t i = 0; i < vs.size(); i++) {
                native_code << f_name << "Type"
                            << " " << toEnumValueName(vs[i].node, vs[i].pipe)
                            << " = " << f_utils.at(f_name).repr << ";" << endl;
            }
        } else if (isConstructableObject(f_utils.at(f_name))) {
            for (size_t i = 0; i < vs.size(); i++) {
                native_code << type_name(*f_utils.at(f_name).callable_type)
                            << " " << toEnumValueName(vs[i].node, vs[i].pipe)
                            << " = " << f_utils.at(f_name).repr << ";" << endl;
            }
        } else {
            native_code << "std::array<std::function<"
                        << f_utils.at(f_name).arg_types_repr << ">, "
                        << vs.size() << "> " << f_name << "s;" << endl;
            for (size_t i = 0; i < vs.size(); i++) {
                native_code << "std::function<"
                            << f_utils.at(f_name).arg_types_repr << ">& "
                            << toEnumValueName(vs[i].node, vs[i].pipe) << " = "
                            << f_name << "s[" << i << "];" << endl;
            }
        }
        native_code << endl;
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
