#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "function_node.h"
#include "variable.h"

namespace fase {

struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string func_repr;    // Corresponding to function representation
    std::vector<Link> links;  // size == |function arguments|
    std::vector<Variable> arg_values;  // size == |function arguments|
};

struct Function {
    std::unique_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> arg_reprs;        // size == |function arguments|
    std::vector<Variable> default_arg_values;  // size == |function arguments|
};

class FaseCore {
public:
    FaseCore() {}

    template <typename... Args>
    void addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_reprs,
            const std::array<Variable, sizeof...(Args)>& default_args = {});

    bool makeNode(const std::string& node_name, const std::string& func_repr);

    void delNode(const std::string& name) noexcept;

    bool linkNode(const std::string& src_node_name, const size_t& src_arg_idx,
                  const std::string& dst_node_name, const size_t& dst_arg_idx);

    bool setNodeArg(const std::string& node_name, const size_t arg_idx,
                    Variable arg);

    const std::map<std::string, Node>& getNodes();

    const std::map<std::string, Function>& getFunctions();

    bool build();
    bool run();

private:
    // Registered functions
    std::map<std::string, Function> functions;  // [repr, Function]

    // Function nodes
    std::map<std::string, Node> nodes;  // [name, Node]

    // Built pipeline
    std::vector<std::function<void()>> pipeline;
    std::map<std::string, std::vector<Variable>> output_variables;
};

}  // namespace fase

#include "core_impl.h"

#endif  // CORE_H_20180622
