
#include "core.h"

#include <algorithm>

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

bool checkDepends(const fase::Node& node,
                  const std::vector<std::string> binded) {
    for (auto& link : node.links) {
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

bool FaseCore::makeNode(const std::string& name, const std::string& function) {
    // check defined function name.
    if (!exists(functions, function)) return false;

    // check uniqueness of name.
    if (exists(nodes, name) or exists(arguments, name)) {
        return false;
    }

    Node node;
    node.name = name;
    node.function = function;
    node.links.resize(functions[function].arg_names.size());

    nodes[name] = std::move(node);

    return true;
}

bool FaseCore::build() {
    using std::begin;
    using std::end;

    std::vector<std::string> binded;
    std::vector<std::vector<Variable*>> binded_infos;

    pipeline.clear();
    variables.clear();

    variables.resize(arguments.size());

    for (auto& v : arguments) {
        binded.push_back(v.first);
    }
    for (auto& variable : variables) {
        binded_infos.push_back({&variable});
    }

    if (arguments.size() != 0) {
        // bind init function
        pipeline.emplace_back([this]() {
            auto vnode_i = begin(arguments);
            auto variable_i = begin(variables);
            for (; vnode_i != end(arguments); vnode_i++, variable_i++) {
                vnode_i->second.val.copy(*variable_i);
            }
        });
    }

    for (size_t prev_size = binded.size(); true;) {
        for (auto& node : nodes) {
            // check dependency of function node.
            if (!checkDepends(node.second, binded)) continue;

            Function& info = functions[node.second.function];

            std::vector<Variable*> bind_val;
            for (size_t i = 0; i < node.second.links.size(); i++) {
                auto& link_node = node.second.links.at(i).linking_node;

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
                    std::min(size_t(node.second.links.at(i).linking_idx),
                             size_t(binded_infos.at(j).size() - 1))));
            }

            pipeline.emplace_back(info.builder->build(bind_val));

            binded.emplace_back(node.first);
            binded_infos.push_back(bind_val);
        }

        if (binded.size() == nodes.size() + 2) {
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

}  // namespace fase
