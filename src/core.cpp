
#include "core.h"

namespace {

constexpr int kMAX_ID = 1073741824;
constexpr int kMIN_ID = 2;

}  // namespace

namespace fase {

namespace pe {

// TODO check Variable copy !!!
void FaseCore::addVariable(const std::string& name, const Variable& val) {
    inputs.push_back(std::tuple<std::string, Variable>(name, val));
}

void FaseCore::addFunction(const std::string& name,
                           std::unique_ptr<FunctionBuilder>&& f,
                           const std::vector<std::string>& argnames) {
    functions.insert(
        std::make_pair(name, std::make_tuple(std::move(f), argnames)));
}

bool FaseCore::makeFunctionNode(const std::string& f_name) {
    // undefined function name
    if (functions.find(f_name) == std::end(functions)) return false;
    // too many FNode
    if (fnodes.size() >= kMAX_ID - kMIN_ID) return false;

    static int idCounter = kMIN_ID;

    int next_id = idCounter;

    for (; fnodes.find(next_id) != std::end(fnodes); next_id++) {
        if (next_id > kMAX_ID) next_id = kMIN_ID;
    }
    idCounter = next_id + 1;

    FNode node;
    node.func_name = f_name;

    size_t nargs = std::get<1>(functions[f_name]).size();
    for (size_t i = 0; i < nargs; i++) {
        node.dsts.push_back(std::make_shared<VNode>(next_id, int(i)));
    }

    fnodes.emplace(next_id, std::move(node));

    return true;
}

FaseCore::NdataT FaseCore::getNodeData(const int& index) {
    const FNode& node = fnodes.at(index);
    std::string name = node.func_name;
    return NdataT(index, name, std::get<1>(functions[name]));
}

template <template <class...> class ListClass>
auto FaseCore::getNodeDataList() noexcept {
    ListClass<NdataT> dst;
    for (auto& node : fnodes) {
        std::string name = node.second.func_name;
        dst.emplace_back(node.first, name, std::get<1>(functions[name]));
    }
    return dst;
}

template <template <class...> class ListClass>
auto FaseCore::getFNodeArgLink(const int& index) noexcept {
    const FNode& node = fnodes.at(index);
    ListClass<std::tuple<int, int>> dst;
    for (const auto& v : node.args) {
        if (v.expired()) {
            dst.emplace_back(kNULL_ID, 0);
        } else {
            dst.emplace_back(v.lock()->parentID, v.lock()->arg_i);
        }
    }
}

template <template <class...> class LC1, template <class...> class LC2>
auto FaseCore::getFNodeArgLinkList() noexcept {
    LC1<LC2<std::tuple<int, int>>> dst;
    for (const auto& node : fnodes) {
        LC2<std::tuple<int, int>> list;
        for (const auto& v : node.second.args) {
            if (v.expired()) {
                list.emplace_back(kNULL_ID, 0);
            } else {
                list.emplace_back(v.lock()->parentID, v.lock()->arg_i);
            }
        }
        dst.emplace_back(std::move(list));
    }
    return dst;
}

bool FaseCore::run() {
    // TODO

    return true;
}

}  // namespace pe

}  // namespace fase
