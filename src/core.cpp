
#include "core.h"

#include <algorithm>

namespace fase {

namespace pe {

// TODO check Variable copy !!!
void FaseCore::addVariableBuilder(const std::string& name,
                                  std::function<Variable()>&& vb) {
    variable_builders.insert(std::make_pair(name, vb));
}

void FaseCore::addFunctionBuilder(const std::string& name,
                                  std::unique_ptr<FunctionBuilder>&& f,
                                  const std::vector<std::string>& argnames) {
    functions.insert(
        std::make_pair(name, std::make_tuple(std::move(f), argnames)));
}

bool FaseCore::makeFunctionNode(const std::string& f_name) {
    // undefined function name
    if (functions.find(f_name) == std::end(functions)) return false;
    // too many FNode
    if (fnodes.size() >= kMAX_FNODE_ID - 10) return false;

    static int idCounter = kNULL_FNODE_ID + 1;

    int next_id = idCounter;

    for (; fnodes.find(next_id) != std::end(fnodes) or next_id == kINPUT_ID or
            next_id == kCONST_ID; next_id++) {
        if (next_id > kMAX_FNODE_ID) {
            next_id = kNULL_FNODE_ID + 1;
        }
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

void FaseCore::linkNode(int linked_id, int linked_arg_idx,
                        int link_id, int link_arg_idx) {
    auto& linked = fnodes.at(linked_id);
    auto& linking = fnodes.at(link_id);

    auto& linking_arg = linking.args.at(link_arg_idx);

    if (!linking_arg.expired()) {
        linking_arg.lock()->referenced_c--;
    }
    linking_arg = linked.dsts.at(linked_arg_idx);
    linking_arg.lock()->referenced_c++;
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
            dst.emplace_back(kNULL_FNODE_ID, 0);
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
                list.emplace_back(kNULL_FNODE_ID, 0);
            } else {
                list.emplace_back(v.lock()->parentID, v.lock()->arg_i);
            }
        }
        dst.emplace_back(std::move(list));
    }
    return dst;
}

bool FaseCore::build() {

    std::vector<int> binded{kINPUT_ID, kCONST_ID};

    pipeline.clear();
    variables.clear();

    variables.resize(inputs.size() + constants.size());

    pipeline.emplace_back([this]() {
        for (size_t i = 0; i < inputs.size(); i++) {
            std::get<1>(inputs[i]).copy(variables[i]);
        }
    });

    pipeline.emplace_back([this]() {
        for (size_t i = 0; i < constants.size(); i++) {
            std::get<1>(constants[i]).copy(variables[i + constants.size()]);
        }
    });

    for(size_t prev_size = binded.size(); true;) {
        for ( auto& fnode : fnodes ) {
            // check dependency of function node.
            if (!std::any_of(std::begin(fnode.second.args),
                             std::end(fnode.second.args),
                             [&binded](std::weak_ptr<VNode>& vnode) {
                                return std::any_of(std::begin(binded), std::end(binded),
                                        [id = vnode.lock()->parentID] (int i) {
                                            return i == id;
                                        });
                             })) {
                continue;
            }
            // TODO bind function node, and add it to pipeline.

            binded.emplace_back(fnode.first);
        }

        if (binded.size() == fnodes.size() + 2) {
            return true;
        }

        if (prev_size == binded.size()) {
            // there may be a circular reference or function node having unset argument.
            return false;
        }
    }
}

bool FaseCore::run() {
    // TODO

    return true;
}

}  // namespace pe

}  // namespace fase
