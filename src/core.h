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

struct Argument {
    std::string name;
    Variable val;
    bool constant;
};

struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string name;
    std::string func_name;  // function name (not a node name)

    std::vector<Link> links;
};

struct Function {
    std::unique_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> arg_names;
};

class FaseCore {
public:
    FaseCore() {}

    /// for unset argument
    template <typename T>
    void addVariableBuilder(std::function<Variable()>&& builder) {
        variable_builders[typeid(T).name()] = builder;
    }

    template <typename T>
    void addVariableBuilder() {
        variable_builders[typeid(T).name()] = []() -> Variable {
            Variable v;
            v.create<T>();
            return v;
        };
    }

    template <typename... Args>
    void addFunctionBuilder(
            const std::string& name, std::function<void(Args...)>&& callable,
            const std::array<std::string, sizeof...(Args)>& argnames) {
        func_builders[name] = {
            .builder = std::make_unique<FunctionBuilder<Args...>>(callable),
            .arg_names = std::vector<std::string>(std::begin(argnames),
                                                  std::end(argnames))
        };
    }

    bool makeNode(const std::string& node_name,
                  const std::string& function_name);

    template <typename T>
    bool setArgument(const std::string& name, const bool& is_constant,
                     T&& value) {
        arguments[name] = {name, Variable(std::move(value)), is_constant};
        return true;
    }

    void delNode(const std::string& name) noexcept { nodes.erase(name); }

    void delArgument(const std::string& name) noexcept {
        arguments.erase(name);
    }

    void linkNode(const std::string& src_node_name, const size_t& src_arg_idx,
                  const std::string& dst_node_name, const size_t& dst_arg_idx) {
        nodes[src_node_name].links[src_arg_idx] = {dst_node_name, dst_arg_idx};
    };

    const std::map<std::string, Argument>& getArguments() { return arguments; };
    const std::map<std::string, Node>& getNodes() { return nodes; };

    const std::map<std::string, Function>& getFunctions() {
        return func_builders;
    }

    bool build();
    bool run();

private:
    // input data
    std::map<std::string, std::function<Variable()>> variable_builders;
    std::map<std::string, Function> func_builders;

    // function node data
    std::map<std::string, Node> nodes;
    std::map<std::string, Argument> arguments;

    // built pipeline
    std::vector<std::function<void()>> pipeline;
    std::list<Variable> variables;  // for running.
};

}  // namespace fase

#endif  // CORE_H_20180622
