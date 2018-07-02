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

class FaseCore {
public:
    FaseCore() {}

    template <typename T>
    void addDefaultConstructor(
            const std::string& name,
            std::function<Variable()>&& builder = []()->Variable{return T();}) {
        type_table[typeid(T).name()] = name;
        constructors[name] = builder;
    }

    template <typename... Args>
    void addFunctionBuilder(const std::string& name,
                            std::function<void(Args...)>&& callable) {
        func_builders[name] =
                std::make_unique<FunctionBuilder<Args...>>(callable);
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

    const std::map<std::string, std::unique_ptr<FunctionBuilderBase>>&
            getFunctions() {
        return func_builders;
    }

    bool build();
    bool run();

private:
    // input data
    std::map<std::string, std::string> type_table;
    std::map<std::string, std::function<Variable()>> constructors;
    std::map<std::string, std::unique_ptr<FunctionBuilderBase>> func_builders;

    // function node data
    std::map<std::string, Node> nodes;
    std::map<std::string, Argument> arguments;

    // built pipeline
    std::vector<std::function<void()>> pipeline;
    std::list<Variable> variables;  // for running.
};

}  // namespace fase

#endif  // CORE_H_20180622
