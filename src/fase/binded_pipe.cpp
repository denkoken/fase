#include "binded_pipe.h"

#include <chrono>

#include "core.h"

namespace fase {

BindedPipeline::BindedPipeline(const FaseCore& core, const Pipeline& pipeline)
    : functions(&core.getFunctions()), binding_pipeline(&pipeline) {}

BindedPipeline::~BindedPipeline() {}

void BindedPipeline::init(const FaseCore& core, const Pipeline& pipeline) {
    functions = &core.getFunctions();
    binding_pipeline = &pipeline;
}

std::function<void()> BindedPipeline::build(
        const std::vector<Variable*>& in_args) const {
    return build(in_args, nullptr);
}

std::function<void()> BindedPipeline::build(
        const std::vector<Variable*>& in_argps, ResultReport* p_report) const {
    const Node& in_n = binding_pipeline->nodes.at(InputNodeStr());
    const Node& out_n = binding_pipeline->nodes.at(OutputNodeStr());

    const size_t n_args = in_n.links.size() + out_n.links.size();
    if (in_argps.size() != n_args) {
        throw(InvalidArgN(n_args, in_argps.size()));
    };

    std::vector<std::function<void()>> funcs;
    auto variables =
            std::make_shared<std::map<std::string, std::vector<Variable>>>();

    if (p_report != nullptr) {
        BuildPipeline(binding_pipeline->nodes, *functions, false, false, &funcs,
                      variables.get(), &p_report->child_reports);
    } else {
        BuildPipeline(binding_pipeline->nodes, *functions, false, false, &funcs,
                      variables.get(), nullptr);
    }

    auto in_args = std::make_shared<std::vector<Variable>>();
    for (auto p : in_argps) {
        in_args->emplace_back(*p);
    }

    return [funcs = std::move(funcs), variables, in_args,
            n_input = in_n.links.size(), n_output = out_n.links.size(),
            p_report] {
        auto start = std::chrono::system_clock::now();

        for (size_t i = 0; i < n_input; i++) {
            (*in_args)[i].copy((*variables)[InputNodeStr()][i]);
        }
        for (auto& func : funcs) {
            func();
        }
        for (size_t i = 0; i < n_output; i++) {
            variables->at(OutputNodeStr())[i].copy((*in_args)[n_input + i]);
        }

        if (p_report != nullptr) {
            p_report->execution_time = std::chrono::system_clock::now() - start;
        }
    };
}

std::function<void()> BindedPipeline::buildNoTypeCheckAtRun(
        const std::vector<Variable*>& in_argps) const {
    const Node& in_n = binding_pipeline->nodes.at(InputNodeStr());
    const Node& out_n = binding_pipeline->nodes.at(OutputNodeStr());

    const size_t n_args = in_n.links.size() + out_n.links.size();
    if (in_argps.size() != n_args) {
        throw(InvalidArgN(n_args, in_argps.size()));
    };

    std::vector<std::function<void()>> funcs;
    auto variables =
            std::make_shared<std::map<std::string, std::vector<Variable>>>();

    BuildPipeline(binding_pipeline->nodes, *functions, false, true, &funcs,
                  variables.get(), nullptr);

    auto in_args = std::make_shared<std::vector<Variable>>();
    for (auto p : in_argps) {
        in_args->emplace_back(*p);
    }

    return [funcs = std::move(funcs), variables, in_args,
            n_input = in_n.links.size(), n_output = out_n.links.size()] {
        for (size_t i = 0; i < n_input; i++) {
            (*in_args)[i].copy((*variables)[InputNodeStr()][i]);
        }
        for (auto& func : funcs) {
            func();
        }
        for (size_t i = 0; i < n_output; i++) {
            variables->at(OutputNodeStr())[i].copy((*in_args)[n_input + i]);
        }
    };
}

}  // namespace fase
