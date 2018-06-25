
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

struct FNode;

/// for VNode::parentID
constexpr int kINPUT_ID = 0;
constexpr int kCONST_ID = 1;
constexpr int kMAX_FNODE_ID = 1073741824;
constexpr int kNULL_FNODE_ID = -1;

/// VNode has which parerent of this, and index in parerent's argument.
struct VNode {
    int parentID;
    int arg_i;
    int referenced_c = 0;
    VNode(const int& parent, int idx) : parentID(parent), arg_i(idx) {}

};

struct FNode {
    std::string func_name;
    std::vector<std::shared_ptr<VNode>> dsts;

    std::vector<std::weak_ptr<VNode>> args;
};


class FaseCore {
public:
    FaseCore();

    using FuncInfo =
        std::tuple<std::unique_ptr<FunctionBuilder>, std::vector<std::string>>;

    void addVariableBuilder(const std::string& name, std::function<Variable()>&& vb);

    void addFunctionBuilder(const std::string& name,
                     std::unique_ptr<FunctionBuilder>&& f,
                     const std::vector<std::string>& argnames);

    bool makeFunctionNode(const std::string& f_name);

    void deleteFunctionNode(const int& index) noexcept { fnodes.erase(index); }

    void linkNode(int linked_id, int linked_arg_idx,
                  int link_id, int link_arg_idx);

    int getNodesSize() noexcept { return fnodes.size(); }  // necessary ?

    using NdataT =
        std::tuple<int, std::string, const std::vector<std::string>&>;
    ///
    /// return NdataT a.k.a tuple(index (int), function name (string),
    ///                           ref of argument names (const vector<string>&)
    /// If input undefined index, throw std::out_of_range.
    ///
    NdataT getNodeData(const int& index);  // necessary ?

    ///
    /// return ListClass<NdataT a.k.a
    /// tuple<int (index), string (function name),
    ///       const vector<string>& (ref of argument names)>.
    /// ListClass<T> must have emplace_back(Args&&...).
    ///
    template <template <class...> class ListClass>
    auto getNodeDataList() noexcept;

    ///
    /// return ListClass<tuple<int (function node ID), int (argument index)>.
    /// ListClass<T> must have emplace_back(Args&&...).
    /// If frist of tuple is kNULL_ID, it means that argument is unset.
    ///
    template <template <class...> class ListClass>
    auto getFNodeArgLink(const int& index) noexcept;  // necessary ?

    ///
    /// return LC1<LC2<tuple<int (function node ID), int (argument index)>>.
    /// LC1 and LC2 must have emplace_back(Args&&...).
    /// If frist of tuple is kNULL_ID, it means that argument is unset.
    ///
    template <template <class...> class LC1, template <class...> class LC2>
    auto getFNodeArgLinkList() noexcept;

    template <template <class...> class ListClass>
    auto getFuncNames() noexcept {
        ListClass<std::string> keys;
        for (auto& item : functions) {
            keys.push_back(item.first);
        }
        return keys;
    }

    bool build();
    bool run();

private:
    // input data
    std::map<std::string, std::function<Variable()>> variable_builders;
    std::map<std::string, const FuncInfo> functions;  // TODO map or unordered ?

    // constant data
    std::vector<std::tuple<std::string, Variable>> constants;

    // function node data
    std::unordered_map<int, FNode> fnodes;

    std::vector<std::shared_ptr<VNode>> input_vnodes;
    std::vector<std::shared_ptr<VNode>> const_vnodes;

    // built pipeline
    std::vector<std::function<void()>> pipeline;
    std::vector<Variable> variables;
};

}  // namespace pe

}  // namespace fase

#endif  // CORE_H_20180622
