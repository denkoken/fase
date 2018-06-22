
#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "function_node.h"
#include "variable.h"

namespace fase {

namespace pe {

struct FNode;

/// VNode has which parerent of this, and index in parerent's argument.
struct VNode {
    std::weak_ptr<FNode> parent;
    int arg_i;
    VNode(std::weak_ptr<FNode> parent, int idx) : parent(parent), arg_i(idx) {}
};

struct FNode {
    std::string func_name;
    std::vector<std::shared_ptr<const VNode>> dsts;

    std::vector<std::weak_ptr<const VNode>> args;
};

class FaseCore {
public:
    FaseCore();

    using FuncInfo =
        std::tuple<std::unique_ptr<FunctionBuilder>, std::vector<std::string>>;

    void addVariable(const std::string& name, const Variable& val);

    void addFunction(const std::string& name,
                     std::unique_ptr<FunctionBuilder>&& f,
                     const std::vector<std::string>& argnames);

    bool makeFunctionNode(const std::string& f_name);

    void build();

    std::vector<std::string> getFuncNames() {
        std::vector<std::string> keys;
        for (auto& item : functions) {
            keys.push_back(item.first);
        }
        return keys;
    }

private:
    // input data
    std::map<std::string, const Variable> inputs;
    std::map<std::string, const FuncInfo> functions;

    // function node data (! Don't Copy shared_ptr !)
    std::list<std::shared_ptr<FNode>> fnodes;
};

}  // namespace pe

}  // namespace fase

#endif  // CORE_H_20180622
