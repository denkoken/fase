
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
    enum struct Type : char {
        FUNCTION_DST,
        INPUT_VARIABLE,
        CONSTANT,
    };

    Type type;
    std::weak_ptr<FNode> parent;
    int arg_i;
    VNode(std::weak_ptr<FNode> parent, int idx)
        : type(Type::FUNCTION_DST), parent(parent), arg_i(idx) {}
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

    bool outputJson(const std::string& dir);

    int getNodesSize() { return fnodes.size(); }

    template <template <class...> class ListClass>
    auto getNodeDataList() {
        using dataT = std::tuple<std::string, const std::vector<std::string>&>;
        ListClass<dataT> dst;
        for (auto& node : fnodes) {
            std::string name = node->func_name;
            dst.push_back(dataT(name, std::get<1>(functions[name])));
        }
        return dst;
    }

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
    std::vector<std::tuple<std::string, Variable>> inputs;
    std::map<std::string, const FuncInfo> functions;  // TODO map or unsorted ?

    // constant data
    std::vector<std::tuple<std::string, Variable>> constants;

    // function node data (! Don't Copy shared_ptr !)
    std::list<std::shared_ptr<FNode>> fnodes;  // TODO vector or list ?
};

}  // namespace pe

}  // namespace fase

#endif  // CORE_H_20180622
