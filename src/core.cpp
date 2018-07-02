#include "core.h"

#include <algorithm>

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <typename T>
inline bool exists(const std::vector<T>& vec, const T& key) {
    return std::find(std::begin(vec), std::end(vec), key) != std::end(vec);
}

bool checkDepends(const fase::Node& node,
                  const std::vector<std::string> binded) {
    for (auto& link : node.links) {
        if (link.node_name == std::string("")) continue;  // unset arg

        if (std::find(std::begin(binded), std::begin(binded), link.node_name) !=
            std::begin(binded)) {
            return false;
        }
    }
    return true;
}

}  // namespace

namespace fase {

bool FaseCore::makeNode(const std::string& name, const std::string& func_name) {
    // check defined function name.
    if (!exists(func_builders, func_name)) return false;

    // check uniqueness of name.
    if (exists(nodes, name) or exists(arguments, name)) {
        return false;
    }

    size_t n_args = func_builders[func_name]->getArgTypes().size();

    nodes[name] = {.name = name,
                   .func_name = func_name,
                   .links = std::vector<Link>(n_args)};

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

            if (exists(binded, node.first)) continue;

            std::unique_ptr<FunctionBuilderBase>& builder =
                    func_builders[node.second.func_name];
            const std::vector<std::string> arg_types = builder->getArgTypes();

            std::vector<Variable*> bind_val;
            for (size_t i = 0; i < node.second.links.size(); i++) {
                auto& link_node = node.second.links.at(i).node_name;

                if (link_node == std::string("")) {  // make variable
                    const std::string& type_name = type_table[arg_types.at(i)];
                    variables.emplace_back(constructors.at(type_name)());
                    bind_val.push_back(&variables.back());
                    continue;
                }
                size_t j = size_t(
                        std::find(begin(binded), end(binded), link_node) -
                        begin(binded));
                bind_val.push_back(binded_infos.at(j).at(
                        std::min(size_t(node.second.links.at(i).arg_idx),
                                 size_t(binded_infos.at(j).size() - 1))));
            }

            pipeline.emplace_back(builder->build(bind_val));

            binded.emplace_back(node.first);
            binded_infos.push_back(bind_val);
        }

        if (binded.size() == nodes.size() + 1) {
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
