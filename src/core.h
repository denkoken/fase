#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <cassert>

#include "function_node.h"
#include "variable.h"

namespace fase {

struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string func_repr;   // Corresponding to function representation
    std::vector<Link> links;                   // size == |function arguments|
    std::vector<Variable> default_arg_values;  // size <= |function arguments|
};

struct Function {
    std::unique_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> arg_reprs;        // size == |function arguments|
    std::vector<Variable> default_arg_values;  // size <= |function arguments|
};


template <std::size_t N>
void CreateDefaultArgs(const std::array<Variable, N>& src_args,
                       std::array<Variable, N>& dst_args, size_t idx) {
}

template <std::size_t N, typename Head, typename... Tail>
void CreateDefaultArgs(const std::array<Variable, N>& src_args,
                       std::array<Variable, N>& dst_args, size_t idx = 0) {
    if (src_args[idx].template isSameType<Head>()) {
        // Use input argument
        dst_args[idx] = src_args[idx];
    } else if (src_args[idx].template isSameType<void>()) {
        // Create by default constructor
        using Type = typename std::remove_reference<Head>::type;
        dst_args[idx] = Type();
    } else {
        // Invalid type
        assert(false);
    }

    // Recursive call
    CreateDefaultArgs<N, Tail...>(src_args, dst_args, idx + 1);
}

class FaseCore {
public:
    FaseCore() {}

    template <typename... Args>
    void addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_reprs,
            const std::array<Variable, sizeof...(Args)>& default_args = {}) {

        std::array<Variable, sizeof...(Args)> args;
        CreateDefaultArgs<sizeof...(Args), Args...>(default_args, args);

        functions[func_repr] = {
                std::make_unique<FunctionBuilder<Args...>>(func_val),
                std::vector<std::string>(arg_reprs.begin(), arg_reprs.end()),
                std::vector<Variable>(args.begin(), args.end())
        };
    }

    bool makeNode(const std::string& node_name, const std::string& func_repr);

    void delNode(const std::string& name) noexcept { nodes.erase(name); }

    void linkNode(const std::string& src_node_name, const size_t& src_arg_idx,
                  const std::string& dst_node_name, const size_t& dst_arg_idx) {
        nodes[dst_node_name].links[dst_arg_idx] = {src_node_name, src_arg_idx};
    };

    template<typename... DefaultArgs>
    bool setDefaultArgs(const std::string& node_name,
                        DefaultArgs&&... default_args) {
        if (nodes.find(node_name) != std::end(nodes)) {
            return false;
        }
        nodes[node_name].default_arg_values = { Variable(default_args)... };
        return true;
    }

    const std::map<std::string, Node>& getNodes() { return nodes; }

    const std::map<std::string, Function>& getFunctions() {
        return functions;
    }

    bool build();
    bool run();

private:
    // input data
    std::map<std::string, Function> functions;  // [repr, Function]

    // function node data
    std::map<std::string, Node> nodes;  // [name, Node]

    // built pipeline
    std::vector<std::function<void()>> pipeline;
    std::map<std::string, std::vector<Variable>> output_variables;
};

}  // namespace fase

#endif  // CORE_H_20180622
