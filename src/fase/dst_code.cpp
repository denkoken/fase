#include "core_util.h"

#include <sstream>

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

std::string genFunctionCall(const std::string& func_repr,
                            const std::vector<std::string>& var_names) {
    // Add declaration code (Remove reference for declaration)
    std::stringstream ss;
    ss << func_repr << "(";
    for (size_t i = 0; i < var_names.size(); i++) {
        ss << var_names[i];
        if (i != var_names.size() - 1) {
            ss << ", ";
        }
    }
    ss << ");" << std::endl;
    return ss.str();
}

std::string getValStr(const Variable& v, const TypeUtils& utils) {
    for (const auto& pair : utils.checkers) {
        if (std::get<1>(pair)(v)) {
            return utils.def_makers.at(std::get<0>(pair))(v);
        }
    }
    return "";
}

}  // namespace

std::string GenNativeCode(const FaseCore& core, const TypeUtils& utils,
                          const std::string& entry_name,
                          const std::string& indent) {
    std::stringstream native_code;

    // Stack for finding runnable node
    auto node_order = GetCallOrder(core.getNodes());

    std::string func_name = entry_name;
    if (func_name.empty()) {
        func_name = "Pipeline";
    }

    native_code << "void " << func_name << "(";
    // Input Arguments
    if (!core.getNodes().at(InputNodeStr()).links.empty()) {
        const std::vector<std::string>& arg_type_reprs =
                core.getFunctions()
                        .at(InputFuncStr(core.getCurrentPipelineName()))
                        .arg_type_reprs;
        const std::vector<std::string>& names =
                core.getFunctions()
                        .at(InputFuncStr(core.getCurrentPipelineName()))
                        .arg_names;
        size_t n_args = core.getNodes().at(InputNodeStr()).links.size();

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            native_code << genVarDeclaration(arg_type_reprs[arg_idx], "",
                                             names[arg_idx], "");
            if (arg_idx != n_args - 1) {
                native_code << ", ";
            }
        }
    }
    // Output Arguments
    if (!core.getFunctions()
                 .at(OutputFuncStr(core.getCurrentPipelineName()))
                 .arg_names.empty()) {
        const Function& out_f = core.getFunctions().at(
                OutputFuncStr(core.getCurrentPipelineName()));
        size_t n_args = out_f.arg_names.size();

        if (!core.getNodes().at(InputNodeStr()).links.empty()) {
            native_code << ", ";
        }

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            native_code << removeReprConst(
                                   removeReprRef(out_f.arg_type_reprs[arg_idx]))
                        << "& " << out_f.arg_names[arg_idx];
            if (arg_idx != n_args - 1) {
                native_code << ", ";
            }
        }
    }
    native_code << ") {" << std::endl;

    while (true) {
        // Find runnable node
        const std::string node_name = PopFront(node_order);
        if (node_name.empty()) {
            break;
        }

        if (node_name.find(ReportHeaderStr()) != std::string::npos) {
            continue;
        }

        const Node& node = core.getNodes().at(node_name);
        const size_t n_args = node.links.size();

        // Add comment
        native_code << indent;
        native_code << "// " << node.func_repr << " [" << node_name << "]"
                    << std::endl;

        // Argument representations
        const std::vector<std::string>& arg_type_reprs =
                core.getFunctions().at(node.func_repr).arg_type_reprs;
        std::vector<std::string> arg_reprs;
        for (const auto& v : node.arg_values) {
            arg_reprs.emplace_back(getValStr(v, utils));
        }

        // Collect argument names
        std::vector<std::string> var_names;
        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            if (link.node_name.empty()) {
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
                    var_names.push_back(
                            core.getFunctions()
                                    .at(InputFuncStr(
                                            core.getCurrentPipelineName()))
                                    .arg_names.at(link.arg_idx));
                } else {
                    var_names.push_back(
                            genVarName(link.node_name, link.arg_idx));
                }
            }
        }

        // Add function call
        native_code << indent;
        native_code << genFunctionCall(node.func_repr, var_names);
    }
    assert(node_order.empty());

    // Set output
    if (!core.getFunctions()
                 .at(OutputFuncStr(core.getCurrentPipelineName()))
                 .arg_names.empty()) {
        const Function& out_f = core.getFunctions().at(
                OutputFuncStr(core.getCurrentPipelineName()));
        size_t n_args = out_f.arg_names.size();
        const Node& node = core.getNodes().at(OutputNodeStr());

        for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
            auto& link = node.links[arg_idx];
            native_code << indent;
            native_code << out_f.arg_names[arg_idx] << " = "
                        << genVarName(link.node_name, link.arg_idx) << ";"
                        << std::endl;
        }
    }

    native_code << "}";

    return native_code.str();
}

}  // namespace fase
