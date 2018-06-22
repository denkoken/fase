
#include "core.h"

namespace fase {

namespace pe {

void FaseCore::addVariable(const std::string& name, const Variable& val) {
    inputs.insert(std::make_pair(name, val));
}

void FaseCore::addFunction(const std::string& name,
                           std::unique_ptr<FunctionBuilder>&& f,
                           const std::vector<std::string>& argnames) {
    functions.insert(
        std::make_pair(name, std::make_pair(std::move(f), argnames)));
}

bool FaseCore::makeFunctionNode(const std::string& f_name) {
    if (functions.find(f_name) == std::end(functions)) return false;

    std::shared_ptr<FNode> node = std::make_shared<FNode>();
    node->func_name = f_name;

    size_t nargs = std::get<1>(functions.at(f_name)).size();
    for (size_t i = 0; i < nargs; i++) {
        node->dsts.push_back(std::make_shared<VNode>(node, int(i)));
    }

    fnodes.emplace_back(node);

    return true;
}

}  // namespace pe

}  // namespace fase
