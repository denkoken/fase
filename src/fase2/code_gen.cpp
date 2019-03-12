
#include "common.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.h"
#include "manager.h"

namespace fase {

using std::string, std::vector, std::map, std::type_index;
using size_t = size_t;

class ENDL {};

static ENDL endl;

class MyStream {
public:
    MyStream(const string& indent_) : indent(indent_) {}

    template <typename T>
    MyStream& operator<<(T&& v) {
        if constexpr (std::is_same_v<ENDL, std::decay_t<T>>) {
            newLine();
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

namespace {

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

string getValStr(const Variable& v, const TSCMap& tsc_map) {
    if (!tsc_map.count(v.getType())) {
        return "";
    }
    return tsc_map.at(v.getType()).def_maker(v);
}

vector<string> genLocalVariableDef(MyStream& native_code,
                                   const PipelineAPI& pipe_api,
                                   const string& node_name,
                                   const TSCMap& tsc_map) {
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
        auto [src, s_idx] = GetSrcName(node_name, arg_idx, pipe_api.getLinks());
        if (src.empty() && function.is_input_args[arg_idx]) {
            var_names.push_back(arg_reprs[arg_idx]);
        } else if (src.empty()) {
            // Case 1: Create default argument
            var_names.push_back(genVarName(node_name, arg_idx));
            // Add declaration code
            native_code << genVarDeclaration(arg_type_reprs[arg_idx],
                                             arg_reprs[arg_idx],
                                             var_names.back())
                        << endl;
        } else {
            // Case 2: Use output variable
            if (src == InputNodeName()) {
                const FunctionUtils& in_f = functions.at(kInputFuncName);
                var_names.push_back(in_f.arg_names.at(s_idx));
            } else {
                var_names.push_back(genVarName(src, s_idx));
            }
        }
    }
    return var_names;
}

void genFuncDeclaration(MyStream& code_stream, const string& func_name,
                        const map<string, Node>& nodes,
                        const map<string, FunctionUtils> functions,
                        const TSCMap& tsc_map) {
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
                     const FunctionUtils& func_util,
                     const vector<string>& var_names) {
    if (func_util.pure) {
        ss << func_name;
    } else {
        ss << func_name << "s[" << toEnumValueName(node_name, pipe_name) << "]";
    }
    ss << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << var_names[i];
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << endl;
}

void genOutputAssignment(MyStream& native_code, const map<string, Node>& nodes,
                         const map<string, FunctionUtils>& functions,
                         const vector<Link>& links) {
    const Node& out_n = nodes.at(OutputNodeName());
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
            string val_name;
            if (src == InputNodeName()) {
                const Node& in_n = nodes.at(InputNodeName());
                const FunctionUtils& in_f = functions.at(in_n.func_name);
                val_name = in_f.arg_names[s_idx];
            } else {
                val_name = genVarName(src, s_idx);
            }
            native_code << out_f.arg_names[arg_idx] << " = " << val_name << ";";
        }
    }
}

bool genFunctionCode(MyStream& native_code, const string& p_name,
                     const CoreManager& cm, const TSCMap& tsc_map,
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
    native_code << "{";
    native_code.upIndentLevel();
    native_code << endl;

    while (true) {
        // Find runnable node
        const string n_name = PopFront(node_order);
        if (n_name.empty()) {
            break;
        }

        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            continue;
        }
        // if (n_name.find(ReportHeaderStr()) != string::npos) {
        //     continue;
        // }
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

    genOutputAssignment(native_code, nodes, functions, cm[p_name].getLinks());

    native_code.downIndentLevel();
    native_code << endl << "}";

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
            if (!f_utils.at(node.func_name).pure) {
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
        native_code << "std::array<std::function<void"
                    << f_utils.at(f_name).arg_types_repr;
        native_code << ">, " << vs.size() << "> " << f_name + "s;" << endl;

        for (size_t i = 0; i < vs.size(); i++) {
            native_code << "constexpr static int "
                        << toEnumValueName(vs[i].node, vs[i].pipe) << " = " << i
                        << ";" << endl;
        }
    }
}

void GenSetters(MyStream& native_code,
                const map<string, vector<N_ID>>& non_pure_node_map) {
    for (auto& [f_name, vs] : non_pure_node_map) {
        native_code << "template <class Callable>" << endl;
        native_code.upIndentLevel();
        native_code << "void set_" << f_name << "(Callable&& " << f_name
                    << ") {" << endl;
        native_code.upIndentLevel();
        native_code << "for (int i = 0; i < " << vs.size() << "; i++) {"
                    << endl;
        native_code << f_name << "s[i] = " << f_name << ";";
        native_code.downIndentLevel();
        native_code << endl << "}";
        native_code.downIndentLevel();
        native_code << endl << "}" << endl << endl;
    }
}

}  // namespace

string GenNativeCode(const string& p_name, const CoreManager& cm,
                     const TSCMap& tsc_map, const string& entry_name,
                     const string& indent) {
    try {
        MyStream native_code{indent};

        native_code << "class " << entry_name << " {" << endl;
        native_code.upIndentLevel();
        native_code << "private:" << endl;

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
                genFunctionCode(native_code, sub_pipe_name, cm, tsc_map,
                                sub_pipe_name);
                native_code << endl << endl;
            }
        }
        native_code.downIndentLevel();
        native_code << endl << "public:";
        native_code.upIndentLevel();
        native_code << endl;

        GenSetters(native_code, non_pure_node_map);

        // write main pipeline function Definitions.
        genFunctionCode(native_code, p_name, cm, tsc_map, "operator()");

        native_code.downIndentLevel();

        native_code << endl << "};";

        return native_code.str();

    } catch (std::exception& e) {
        std::cerr << "genNativeCore() Error : " << e.what() << std::endl;
        return "Failed to generate code.";
    } catch (...) {
        std::cerr << "genNativeCore() Error : something went wrong."
                  << std::endl;
        return "Failed to generate code.";
    }
}

}  // namespace fase
