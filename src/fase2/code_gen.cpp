
#include "common.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.h"
#include "manager.h"

namespace fase {

using std::string, std::vector, std::map, std::type_index, std::stringstream;
using size_t = size_t;

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

std::string removeReprConst(const std::string& type_repr) {
    const std::string const_str = "const ";
    auto idx = type_repr.rfind(const_str);
    if (idx == std::string::npos) {
        return type_repr;
    } else {
        return type_repr.substr(idx + const_str.size());
    }
}

std::string removeReprRef(const std::string& type_repr) {
    auto idx = type_repr.rfind('&');
    if (idx == std::string::npos) {
        return type_repr;
    } else {
        return type_repr.substr(0, idx);
    }
}

std::string genVarDeclaration(const std::string& type_repr,
                              const std::string& val_repr,
                              const std::string& name,
                              const std::string& footer = ";") {
    // Add declaration code (Remove reference for declaration)
    std::stringstream ss;
    ss << "const " << type_repr << "& " << name;
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

vector<string> genLocalVariableDef(stringstream& native_code,
                                   const PipelineAPI& pipe_api,
                                   const string& node_name,
                                   const TSCMap& tsc_map,
                                   const string& indent) {
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
            native_code << indent;
            native_code << genVarDeclaration(arg_type_reprs[arg_idx],
                                             arg_reprs[arg_idx],
                                             var_names.back())
                        << std::endl;
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

void genFuncDeclaration(std::stringstream& code_stream, const string& func_name,
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
                    names[arg_idx], "");
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

void genFunctionCall(stringstream& ss, const string& func_name,
                     const vector<string>& var_names) {
    // Add declaration code (Remove reference for declaration)
    ss << func_name << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << var_names[i];
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << std::endl;
}

void genOutputAssignment(stringstream& native_code,
                         const map<string, Node>& nodes,
                         const map<string, FunctionUtils>& functions,
                         const vector<Link>& links, const string& indent) {
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
            string val_name;
            if (src == InputNodeName()) {
                const Node& in_n = nodes.at(InputNodeName());
                const FunctionUtils& in_f = functions.at(in_n.func_name);
                val_name = in_f.arg_names[s_idx];
            } else {
                val_name = genVarName(src, s_idx);
            }
            native_code << indent;
            native_code << out_f.arg_names[arg_idx] << " = " << val_name << ";"
                        << std::endl;
        }
    }
}

bool genFunctionCode(std::stringstream& native_code, const string& p_name,
                     const CoreManager& cm, const TSCMap& tsc_map,
                     const string& entry_name, const string& indent) {
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
    native_code << "{" << std::endl;

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
        native_code << indent;
        native_code << "// " << func_name << " [" << n_name << "]" << std::endl;

        vector<string> var_names = genLocalVariableDef(native_code, cm[p_name],
                                                       n_name, tsc_map, indent);

        // Add function call
        native_code << indent;
        genFunctionCall(native_code, func_name, var_names);
    }
    assert(node_order.empty());

    genOutputAssignment(native_code, nodes, functions, cm[p_name].getLinks(),
                        indent);

    native_code << "}";

    return true;
}

}  // namespace

string GenNativeCode(const string& p_name, const CoreManager& cm,
                     const TSCMap& tsc_map, const string& entry_name,
                     const string& indent) {
    try {
        std::stringstream native_code;

        auto sub_pipes = cm.getDependingTree().getDependenceLayer(p_name);
        // write sub pipeline function Definitions.
        for (auto iter = sub_pipes.rbegin(); iter != sub_pipes.rend(); iter++) {
            for (auto& sub_pipe_name : *iter) {
                genFunctionCode(native_code, sub_pipe_name, cm, tsc_map,
                                sub_pipe_name, indent);
                native_code << std::endl << std::endl;
            }
        }
        native_code << std::endl << std::endl;

        // write main pipeline function Definitions.
        genFunctionCode(native_code, p_name, cm, tsc_map, entry_name, indent);

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
