
#include "core.h"

#include <algorithm>

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

}

namespace fase {

namespace pe {

template <typename Callable, typename... Args>
void FaseCore::addFunctionBuilder(const std::string& name, Callable func,
                        const std::array<std::string, sizeof...(Args)>& argnames) {
    FunctionInfo info;
    info.builder = std::make_unique<FunctionBinder<Args...>>(func);
    info.arg_names = std::vector<std::string>(std::begin(argnames), 
                                              std::end(argnames));
    info.arg_types = {typeid(Args).name()...};
    func_infos[name] = std::move(info);
}


bool FaseCore::makeFunctionNode(const std::string& name,
                                const std::string& f_name) {
    // check defined function name.
    if (!exists(func_infos, f_name)) return false;

    // check uniqueness of name.
    if (exists(function_nodes, name) or exists(variable_nodes, name)){ 
        return false;
    }

    FunctionNode node = {name, f_name};
    node.links.resize(func_infos[f_name].arg_names.size());

    function_nodes[name] = std::move(node);

    return true;
}


template <typename T>
bool FaseCore::makeVariableNode(const std::string& name,
                                const bool& is_constant,
                                T&& value) {
    variable_nodes[name] = {name, Variable(value), is_constant};
}

bool FaseCore::build() {

    std::vector<int> binded{kINPUT_ID, kCONST_ID};

    pipeline.clear();
    variables.clear();

    variables.resize(input_vnodes.size() + const_vnodes.size());

    pipeline.emplace_back([this]() {
        for (size_t i = 0; i < input_vnodes.size(); i++) {
            std::get<1>(input_vnodes[i]).copy(variables[i]);
        }
    });

    pipeline.emplace_back([this]() {
        for (size_t i = 0; i < const_vnodes.size(); i++) {
            std::get<1>(const_vnodes[i]).copy(variables[i + constants.size()]);
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
