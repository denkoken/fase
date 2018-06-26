
#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

#include "function_node.h"
#include "variable.h"

namespace fase {

namespace pe {

/// VNode has which parerent of this, and index in parerent's argument.
struct VariableNode {
    std::string name;
    Variable val;
    bool constant;
};

struct LinkInfo {
    std::string linking_node;
    int linking_idx;
};

struct FunctionNode {
    std::string name;
    std::string type;  // function name (not a node name)

    std::vector<LinkInfo> links;
};

struct FunctionInfo {
    std::unique_ptr<FunctionBuilder> builder;
    std::vector<std::string> arg_names;

    std::vector<std::string> arg_types;
};


class FaseCore {
public:
    FaseCore();

    /// for unset argument
    template <typename T>
    void addVariableBuilder(std::function<Variable()>&& builder){
        variable_builders[typeid(T).name()] = builder;
    }

    template <typename Callable, typename... Args>
    void addFunctionBuilder(const std::string& name, Callable func,
                            const std::array<std::string, sizeof...(Args)>& argnames);

    bool makeFunctionNode(const std::string& node_name,
                          const std::string& func_name);

    template <typename T>
    bool makeVariableNode(const std::string& name,
                          const bool& is_constant,
                          T&& value);

    void delFunctionNode(const std::string& name) noexcept {
        function_nodes.erase(name);
    }

    void delVariableNode(const std::string& name) noexcept {
        variable_nodes.erase(name);
    }

    void linkNode(const std::string& linking_node, const int& link_idx,
                  const std::string& linked_node, const int& linked_idx){ 
        function_nodes[linking_node].links[link_idx] = {linked_node, linked_idx};
    };

    const std::map<std::string, VariableNode>& getVariableNodes(){ return variable_nodes; };
    const std::map<std::string, FunctionNode>& getFunctionNodes(){ return function_nodes; };

    const std::map<std::string, FunctionInfo>& getFunctionInfos() { return func_infos; }

    bool build();
    bool run();

private:
    // input data
    std::map<std::string, std::function<Variable()>> variable_builders;
    std::map<std::string, FunctionInfo> func_infos;

    // function node data
    std::map<std::string, FunctionNode> function_nodes;
    std::map<std::string, VariableNode> variable_nodes;

    // built pipeline
    std::vector<std::function<void()>> pipeline;
    std::vector<Variable> variables;  // for running.
};

}  // namespace pe

}  // namespace fase

#endif  // CORE_H_20180622
