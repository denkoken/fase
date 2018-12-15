#include "core_util.h"

#include <sstream>
#include <string>
#include <vector>

#include "core.h"

namespace fase {
namespace {
template <typename T>
void extractKeys(const std::map<std::string, T>& src_map,
                 std::set<std::string>& dst_set) {
    for (auto it = src_map.begin(); it != src_map.end(); it++) {
        dst_set.emplace(it->first);
    }
}

template <typename T>
T PopFront(std::vector<std::set<T>>& set_array) {
    if (set_array.empty()) {
        return T();
    }

    auto dst = *std::begin(set_array[0]);
    set_array[0].erase(dst);

    if (set_array[0].empty()) {
        set_array.erase(std::begin(set_array));
    }

    return dst;
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
    ss << removeReprConst(removeReprRef(type_repr)) << " " << name;
    if (!val_repr.empty()) {
        // Add variable initialization
        ss << " = " << val_repr;
    }
    ss << footer;
    return ss.str();
}

std::string FuncName(const std::string& func_repr) {
    if (IsSubPipelineFuncStr(func_repr)) {
        return GetSubPipelineNameFromFuncStr(func_repr);
    }
    return func_repr;
}

void genFunctionCall(std::stringstream& ss, const std::string& func_repr,
                     const std::vector<std::string>& var_names) {
    // Add declaration code (Remove reference for declaration)
    ss << FuncName(func_repr) << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << var_names[i];
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << std::endl;
}

std::string getValStr(const Variable& v, const TypeUtils& utils) {
    for (const auto& pair : utils.checkers) {
        if (std::get<1>(pair)(v)) {
            return utils.def_makers.at(std::get<0>(pair))(v);
        }
    }
    return "";
}

void genFuncDeclaration(std::stringstream& code_stream,
                        const std::string& func_name,
                        const std::map<std::string, Node>& nodes,
                        const std::map<std::string, Function>& functions,
                        const TypeUtils& utils) {
    const Node& in_n = nodes.at(InputNodeStr());
    const Node& out_n = nodes.at(OutputNodeStr());

    const Function& in_f = functions.at(in_n.func_repr);
    const Function& out_f = functions.at(out_n.func_repr);

    code_stream << "void " << func_name << "(";
    // Input Arguments
    if (!nodes.at(InputNodeStr()).links.empty()) {
        const std::vector<const std::type_info*>& arg_types = in_f.arg_types;

        const std::vector<std::string>& names = in_f.arg_names;

        size_t n_args = in_n.links.size();

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            code_stream << genVarDeclaration(utils.names.at(arg_types[arg_idx]),
                                             "", names[arg_idx], "");
            if (arg_idx != n_args - 1) {
                code_stream << ", ";
            }
        }
    }

    // Output Arguments
    if (!out_f.arg_names.empty()) {
        size_t n_args = out_f.arg_names.size();

        if (!nodes.at(InputNodeStr()).links.empty()) {
            code_stream << ", ";
        }

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            code_stream << removeReprConst(removeReprRef(
                                   utils.names.at(out_f.arg_types[arg_idx])))
                        << "& " << out_f.arg_names[arg_idx];
            if (arg_idx != n_args - 1) {
                code_stream << ", ";
            }
        }
    }
    code_stream << ") ";
}

std::vector<std::string> genLocalVariableDef(
        std::stringstream& native_code,
        const std::map<std::string, Node>& nodes,
        const std::map<std::string, Function>& functions,
        const std::string& node_name, const TypeUtils& utils,
        const std::string& indent) {
    const Node& node = nodes.at(node_name);
    const Function& function = functions.at(node.func_repr);

    // Argument representations
    const std::vector<std::string>& arg_type_reprs = [&] {
        std::vector<std::string> dst;
        for (auto& type : function.arg_types) {
            dst.emplace_back(utils.names.at(type));
        }
        return dst;
    }();

    std::vector<std::string> arg_reprs;
    for (const auto& v : node.arg_values) {
        arg_reprs.emplace_back(getValStr(v, utils));
    }

    const size_t n_args = node.links.size();
    std::vector<std::string> var_names;
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        auto& link = node.links[arg_idx];
        if (link.node_name.empty() && function.is_input_args[arg_idx]) {
            var_names.push_back(arg_reprs[arg_idx]);
        } else if (link.node_name.empty()) {
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
            if (link.node_name == InputNodeStr()) {
                const Node& in_n = nodes.at(InputNodeStr());
                const Function& in_f = functions.at(in_n.func_repr);
                var_names.push_back(in_f.arg_names.at(link.arg_idx));
            } else {
                var_names.push_back(genVarName(link.node_name, link.arg_idx));
            }
        }
    }
    return var_names;
}

void genOutputAssignment(std::stringstream& native_code,
                         const std::map<std::string, Node>& nodes,
                         const std::map<std::string, Function>& functions,
                         const std::string& indent) {
    const Node& out_n = nodes.at(OutputNodeStr());
    const Function& out_f = functions.at(out_n.func_repr);

    if (!out_f.arg_names.empty()) {
        size_t n_args = out_f.arg_names.size();
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = out_n.links[arg_idx];
            if (link.node_name.empty()) {
                continue;
            }
            std::string val_name;
            if (link.node_name == InputNodeStr()) {
                const Node& in_n = nodes.at(InputNodeStr());
                const Function& in_f = functions.at(in_n.func_repr);
                val_name = in_f.arg_names[link.arg_idx];
            } else {
                val_name = genVarName(link.node_name, link.arg_idx);
            }
            native_code << indent;
            native_code << out_f.arg_names[arg_idx] << " = " << val_name << ";"
                        << std::endl;
        }
    }
}

bool genFunctionCode(std::stringstream& native_code,
                     const std::map<std::string, Node>& nodes,
                     const std::map<std::string, Function>& functions,
                     const TypeUtils& utils, const std::string& entry_name,
                     const std::string& indent) {
    // Stack for finding runnable node
    auto node_order = GetCallOrder(nodes);

    std::string func_name = entry_name;
    if (func_name.empty()) {
        func_name = "Pipeline";
    }

    genFuncDeclaration(native_code, func_name, nodes, functions, utils);
    native_code << "{" << std::endl;

    while (true) {
        // Find runnable node
        const std::string node_name = PopFront(node_order);
        if (node_name.empty()) {
            break;
        }

        if (node_name.find(ReportHeaderStr()) != std::string::npos) {
            continue;
        }
        const std::string& func_repr = nodes.at(node_name).func_repr;

        // Add comment
        native_code << indent;
        native_code << "// " << func_repr << " [" << node_name << "]"
                    << std::endl;

        std::vector<std::string> var_names = genLocalVariableDef(
                native_code, nodes, functions, node_name, utils, indent);

        // Add function call
        native_code << indent;
        genFunctionCall(native_code, func_repr, var_names);
    }
    assert(node_order.empty());

    genOutputAssignment(native_code, nodes, functions, indent);

    native_code << "}";

    return true;
}

}  // namespace

std::string GenNativeCode(const FaseCore& core, const TypeUtils& utils,
                          const std::string& entry_name,
                          const std::string& indent) noexcept {
    try {
        std::stringstream native_code;

        // write forward declarations of sub pipelines functions.
        for (auto& sub_pipe_name : core.getSubPipelineNames()) {
            auto& nodes = core.getPipelines().at(sub_pipe_name).nodes;
            genFuncDeclaration(native_code, sub_pipe_name, nodes,
                               core.getFunctions(), utils);
            native_code << ";" << std::endl;
        }
        native_code << std::endl << std::endl;

        // write sub pipeline function Definitions.
        for (auto& sub_pipe_name : core.getSubPipelineNames()) {
            auto& nodes = core.getPipelines().at(sub_pipe_name).nodes;
            genFunctionCode(native_code, nodes, core.getFunctions(), utils,
                            sub_pipe_name, indent);
            native_code << std::endl << std::endl << std::endl;
        }
        native_code << std::endl << std::endl << std::endl;

        // write current pipeline function Definitions.
        genFunctionCode(native_code, core.getNodes(), core.getFunctions(),
                        utils, entry_name, indent);

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
