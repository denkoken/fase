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

struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string func_repr;   // Corresponding function representation
    std::vector<Link> links;
};

struct Function {
    std::unique_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> arg_reprs;        // size == |function arguments|
    std::vector<Variable> default_arg_values;  // size <= |function arguments|
};

class FaseCore {
public:
    FaseCore() {}

    template <typename... Args, typename... DefaultArgs>
    void addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_reprs,
            DefaultArgs&&... default_args) {
        // TODO: Type check
        static_assert(sizeof...(DefaultArgs) <= sizeof...(Args),
                      "Too may default argument");
        functions[func_repr] = {
                std::make_unique<FunctionBuilder<Args...>>(func_val),
                std::vector<std::string>(arg_reprs.begin(), arg_reprs.end()),
                { Variable(default_args)... }
        };
    }

    bool makeNode(const std::string& node_name, const std::string& func_repr);

    void delNode(const std::string& name) noexcept { nodes.erase(name); }

    void linkNode(const std::string& src_node_name, const size_t& src_arg_idx,
                  const std::string& dst_node_name, const size_t& dst_arg_idx) {
        nodes[dst_node_name].links[dst_arg_idx] = {src_node_name, src_arg_idx};
    };

    const std::map<std::string, Node>& getNodes() { return nodes; };

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
