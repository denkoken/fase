
#ifndef BINDED_PIPE_H_20180909
#define BINDED_PIPE_H_20180909

#include "core.h"
#include "function_node.h"

namespace fase {
class BindedPipeline : public FunctionBuilderBase {
public:
    BindedPipeline(const FaseCore& core);

    std::function<void()> build(const std::vector<Variable*>& in_args);
    std::function<void()> build(const std::vector<Variable*>& in_args,
                                ResultReport* report);

private:
    Project* project;
};

}  // namespace fase

#endif  // BINDED_PIPE_H_20180909
