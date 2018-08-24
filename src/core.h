#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <algorithm>
#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "function_node.h"
#include "variable.h"

namespace fase {

struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string func_repr;    // Corresponding to function representation
    std::vector<Link> links;  // size == |args|
    std::vector<std::string> arg_reprs;  // size == |args|
    std::vector<Variable> arg_values;    // size == |args|
};

struct Function {
    std::unique_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> arg_type_reprs;       // size == |args|
    std::vector<std::string> default_arg_reprs;    // size == |args|
    std::vector<std::string> arg_names;            // size == |args|
    std::vector<Variable> default_arg_values;      // size == |args|
    std::vector<const std::type_info*> arg_types;  // size == |args| (cleaned)
};

class FaseCore {
public:
    FaseCore() {}

    template <typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<void(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
            const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names = {},
            const std::array<Variable, sizeof...(Args)>& default_args = {});

    bool addNode(const std::string& node_name, const std::string& func_repr);

    void delNode(const std::string& node_name) noexcept;

    bool addLink(const std::string& src_node_name, const size_t& src_arg_idx,
                 const std::string& dst_node_name, const size_t& dst_arg_idx);

    void delLink(const std::string& dst_node_name, const size_t& dst_arg_idx);

    bool setNodeArg(const std::string& node_name, const size_t arg_idx,
                    const std::string& arg_repr, const Variable& arg_val);

    void clearNodeArg(const std::string& node_name, const size_t arg_idx);

    const std::map<std::string, Function>& getFunctions() const;

    const std::map<std::string, Node>& getNodes() const;

    bool build();
    bool run();

    void getRunningOrder(std::vector<std::string>& order);

    template <typename T>
    T getOutput(const std::string& node_name, const size_t& arg_idx,
                const T& default_value = T());

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
