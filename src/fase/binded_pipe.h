
#ifndef BINDED_PIPE_H_20180909
#define BINDED_PIPE_H_20180909

#include <map>

#include "function_node.h"

namespace fase {

struct Pipeline;
struct Function;
class FaseCore;

class BindedPipeline : public FunctionBuilderBase {
public:
    BindedPipeline(const FaseCore& core, const Pipeline& pipeline);
    ~BindedPipeline();

    void init(const FaseCore& core, const Pipeline& pipeline);

    std::function<void()> build(const std::vector<Variable*>& in_args) const;
    std::function<void()> build(const std::vector<Variable*>& in_args,
                                ResultReport* report) const;

private:
    const std::map<std::string, Function>* functions;
    const Pipeline* binding_pipeline;
};

}  // namespace fase

#endif  // BINDED_PIPE_H_20180909
