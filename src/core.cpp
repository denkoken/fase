
#include "core.h"

#include <algorithm>

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

bool checkDepends(const fase::pe::FunctionNode& fnode,
                  const std::vector<std::string> binded) {
    for (auto& link : fnode.links) {
        if (link.linking_node == std::string("")) continue;  // unset arg

        if (std::find(std::begin(binded), std::begin(binded),
                      link.linking_node) != std::begin(binded)) {
            return false;
        }
    }
    return true;
}

}  // namespace

namespace fase {

namespace pe {

bool FaseCore::makeFunctionNode(const std::string& name,
                                const std::string& f_name) {
    // check defined function name.
    if (!exists(func_infos, f_name)) return false;

    // check uniqueness of name.
    if (exists(function_nodes, name) or exists(variable_nodes, name)) {
        return false;
    }

    FunctionNode node;
    node.name = name;
    node.type = f_name;
    node.links.resize(func_infos[f_name].arg_names.size());

    function_nodes[name] = std::move(node);

    return true;
}

bool FaseCore::build() {
    using std::begin;
    using std::end;

    std::vector<std::string> binded;
    std::vector<std::vector<Variable*>> binded_infos;

    pipeline.clear();
    variables.clear();

    variables.resize(variable_nodes.size());

    for (auto& v : variable_nodes) {
        binded.push_back(v.first);
    }
    for (auto& variable : variables) {
        binded_infos.push_back({&variable});
    }

    if (variable_nodes.size() != 0) {
        // bind init function
        pipeline.emplace_back([this]() {
            auto vnode_i = begin(variable_nodes);
            auto variable_i = begin(variables);
            for (; vnode_i != end(variable_nodes); vnode_i++, variable_i++) {
                vnode_i->second.val.copy(*variable_i);
            }
        });
    }

    for (size_t prev_size = binded.size(); true;) {
        for (auto& fnode : function_nodes) {
            // check dependency of function node.
            if (!checkDepends(fnode.second, binded)) continue;

            FunctionInfo& info = func_infos[fnode.second.type];

            std::vector<Variable*> bind_val;
            for (size_t i = 0; i < fnode.second.links.size(); i++) {
                auto& link_node = fnode.second.links.at(i).linking_node;

                if (link_node == std::string("")) {  // make variable
                    variables.emplace_back(
                        variable_builders.at(info.arg_types.at(i))());
                    bind_val.push_back(&variables.back());
                    continue;
                }
                size_t j =
                    size_t(std::find(begin(binded), end(binded), link_node) -
                           begin(binded));
                bind_val.push_back(binded_infos.at(j).at(
                    std::min(size_t(fnode.second.links.at(i).linking_idx),
                             size_t(binded_infos.at(j).size() - 1))));
            }

            pipeline.emplace_back(info.builder->build(bind_val));

            binded.emplace_back(fnode.first);
            binded_infos.push_back(bind_val);
        }

        if (binded.size() == function_nodes.size() + 2) {
            return true;
        }

        if (prev_size == binded.size()) {
            // there may be a circular reference or function node having unset
            // argument.
            return false;
        }
    }
    return false;
}

bool FaseCore::run() {
    // TODO throw catch
    for (auto& f : pipeline) {
        f();
    }
    return true;
}

}  // namespace pe

}  // namespace fase
