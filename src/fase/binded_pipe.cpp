#include "binded_pipe.h"

#include <chrono>

#include "core.h"

namespace fase {

namespace {

auto Build(const std::vector<Variable*>& in_args,
           const std::map<std::string, Function>* functions,
           const Pipeline* pipeline, ResultReport* p_report) {
    const Node& in_n = pipeline->nodes.at(InputNodeStr());
    const Node& out_n = pipeline->nodes.at(OutputNodeStr());

    const size_t n_args = in_n.links.size() + out_n.links.size();
    if (in_args.size() != n_args) {
        throw(InvalidArgN(n_args, in_args.size()));
    };

    std::vector<std::function<void()>> funcs;
    auto variables =
            std::make_shared<std::map<std::string, std::vector<Variable>>>();

    if (p_report != nullptr) {
        BuildPipeline(pipeline->nodes, *functions, false, &funcs,
                      variables.get(), &p_report->child_reports);
    } else {
        BuildPipeline(pipeline->nodes, *functions, false, &funcs,
                      variables.get(), nullptr);
    }

    return [funcs = std::move(funcs), variables, in_args,
            n_input = in_n.links.size(), n_output = out_n.links.size(),
            p_report] {
        auto start = std::chrono::system_clock::now();

        for (size_t i = 0; i < n_input; i++) {
            in_args[i]->copy((*variables)[InputNodeStr()][i]);
        }
        for (auto& func : funcs) {
            func();
        }
        for (size_t i = 0; i < n_output; i++) {
            variables->at(OutputNodeStr())[i].copy(*in_args[n_input + i]);
        }

        if (p_report != nullptr) {
            p_report->execution_time = std::chrono::system_clock::now() - start;
        }
    };
}

}  // namespace

BindedPipeline::BindedPipeline(const FaseCore& core, const Pipeline& pipeline)
    : functions(&core.getFunctions()), binding_pipeline(&pipeline) {}

BindedPipeline::~BindedPipeline() {}

void BindedPipeline::init(const FaseCore& core, const Pipeline& pipeline) {
    functions = &core.getFunctions();
    binding_pipeline = &pipeline;
}

std::function<void()> BindedPipeline::build(
        const std::vector<Variable*>& in_args) const {
    return Build(in_args, functions, binding_pipeline, nullptr);
}

std::function<void()> BindedPipeline::build(
        const std::vector<Variable*>& in_args, ResultReport* report) const {
    return Build(in_args, functions, binding_pipeline, report);
}

}  // namespace fase
