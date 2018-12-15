#include "core.h"

#include <thread>

#include "debug_macros.h"

namespace fase {

namespace {

template <typename T>
using StrKMap = std::map<std::string, T>;

using NodeMap = StrKMap<Node>;
using FuncMap = StrKMap<Function>;

std::vector<Variable*> BindVariables(
        const Node& node,
        const StrKMap<std::vector<Variable>>& exists_variables,
        std::vector<Variable>* variables) {
    assert(variables->empty());

    const size_t n_args = node.links.size();
    // Set output variable
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        const auto& link = node.links[arg_idx];
        if (link.node_name.empty()) {
            // Case 1: Create default argument
            const Variable& v = node.arg_values[arg_idx];
            variables->push_back(v);
        } else {
            // Case 2: Use output variable
            variables->push_back(
                    exists_variables.at(link.node_name)[link.arg_idx]);
        }
    }

    // Collect binding variables
    std::vector<Variable*> bound_variables;
    for (size_t arg_idx = 0; arg_idx < n_args; arg_idx++) {
        bound_variables.push_back(&variables->at(arg_idx));
    }
    return bound_variables;
}

std::function<void()> wrapPipe(const std::string& node_name,
                               std::function<void()>&& f) {
    return [node_name, f = std::forward<std::function<void()>>(f)]() {
        try {
            f();
        } catch (ErrorThrownByNode& e) {
            try {
                e.rethrow_nested();
            } catch (...) {
                throw ErrorThrownByNode(node_name + " :: " + e.node_name,
                                        e.err_message);
            }
        } catch (std::exception& e) {
            throw ErrorThrownByNode(node_name, e.what());
        } catch (std::string& e) {
            throw ErrorThrownByNode(node_name, e);
        } catch (long long e) {
            throw ErrorThrownByNode(node_name, std::to_string(e));
        } catch (...) {
            throw ErrorThrownByNode(node_name, "something went wrong.");
        }
    };
}

std::function<void()> BuildNode(const std::string& node_name,
                                const std::vector<Variable*>& args,
                                const Function& func, const bool opt,
                                StrKMap<ResultReport>* report_box_) {
    if (node_name == InputNodeStr() || node_name == OutputNodeStr()) {
        return [] {};
    }
    std::function<std::function<void()>()> build;
    if (opt) {
        return func.builder->buildNoTypeCheckAtRun(args);
    } else if (report_box_ != nullptr) {
        return wrapPipe(node_name,
                        func.builder->build(args, &(*report_box_)[node_name]));
    } else {
        return wrapPipe(node_name, func.builder->build(args));
    }
}

void BuildNodesParallel(const std::set<std::string>& runnables,
                        const size_t& step,  // for report.
                        const NodeMap& nodes, const FuncMap& functions,
                        const bool opt,
                        StrKMap<std::vector<Variable>>* output_variables,
                        StrKMap<ResultReport>* report_box_,
                        std::vector<std::function<void()>>* built_pipeline) {
    StrKMap<std::vector<Variable*>> variable_ps;
    for (auto& runnable : runnables) {
        variable_ps[runnable] =
                BindVariables(nodes.at(runnable), *output_variables,
                              &(*output_variables)[runnable]);
    }

    // Build
    std::vector<std::function<void()>> funcs;
    for (auto& runnable : runnables) {
        funcs.emplace_back(BuildNode(runnable, variable_ps[runnable],
                                     functions.at(nodes.at(runnable).func_repr),
                                     opt, report_box_));
    }
    if (report_box_ != nullptr) {
        built_pipeline->emplace_back([funcs, report_box_, step] {
            auto start = std::chrono::system_clock::now();
            std::vector<std::thread> ths;
            std::exception_ptr ep;
            for (auto& func : funcs) {
                ths.emplace_back([&ep, &func]() {
                    try {
                        func();
                    } catch (...) {
                        ep = std::current_exception();
                    }
                });
            }
            for (auto& th : ths) {
                th.join();
            }
            if (ep) {
                std::rethrow_exception(ep);
            }
            (*report_box_)[StepStr(step)].execution_time =
                    std::chrono::system_clock::now() - start;
        });
    } else {
        built_pipeline->emplace_back([funcs] {
            std::vector<std::thread> ths;
            std::exception_ptr ep;
            for (auto& func : funcs) {
                ths.emplace_back([&ep, &func]() {
                    try {
                        func();
                    } catch (...) {
                        ep = std::current_exception();
                    }
                });
            }
            for (auto& th : ths) {
                th.join();
            }
            if (ep) {
                std::rethrow_exception(ep);
            }
        });
    }
}

void BuildNodesNonParallel(const std::set<std::string>& runnables,
                           const NodeMap& nodes, const FuncMap& functions,
                           const bool opt,
                           StrKMap<std::vector<Variable>>* output_variables,
                           StrKMap<ResultReport>* report_box_,
                           std::vector<std::function<void()>>* built_pipeline) {
    for (auto& runnable : runnables) {
        const Node& node = nodes.at(runnable);
        auto bound_variables = BindVariables(node, *output_variables,
                                             &(*output_variables)[runnable]);

        // Build
        built_pipeline->emplace_back(BuildNode(
                runnable, bound_variables,
                functions.at(nodes.at(runnable).func_repr), opt, report_box_));
    }
}

bool checkTreeRec(const std::string& pipe, const FaseCore& core,
                  const std::list<std::string>& dependeds,
                  std::list<std::string>& buildable_cache) {
    if (exists(buildable_cache, pipe)) {
        return true;
    }

    const NodeMap& nodes = core.getPipelines().at(pipe).nodes;

    auto child_dependeds = dependeds;
    child_dependeds.push_back(pipe);

    for (auto& pair : nodes) {
        const std::string& f_name = std::get<1>(pair).func_repr;
        if (IsSubPipelineFuncStr(f_name)) {
            auto pipe_name = GetSubPipelineNameFromFuncStr(f_name);

            if (exists(dependeds, pipe_name)) {
                return false;
            }

            bool ret = checkTreeRec(pipe_name, core, child_dependeds,
                                    buildable_cache);
            if (!ret) {  // recursive depending is found.
                return false;
            }
        }
    }

    buildable_cache.push_back(pipe);

    return true;
}

/// check that recursive dependings don't exist.
bool CheckSubPipelineDependencies(const FaseCore& core) {
    std::list<std::string> buildable_cache;
    return checkTreeRec(core.getCurrentPipelineName(), core, {},
                        buildable_cache);
}

}  // namespace

bool BuildPipeline(const NodeMap& nodes, const FuncMap& functions,
                   const bool parallel_exe, const bool opt,
                   std::vector<std::function<void()>>* built_pipeline,
                   StrKMap<std::vector<Variable>>* output_variables,
                   StrKMap<ResultReport>* report_box) {
    // TODO change parallel execution system.

    built_pipeline->clear();
    output_variables->clear();
    if (report_box != nullptr) {
        report_box->clear();
    }

    // Build running order.
    auto node_order = GetCallOrder(nodes);
    assert(!node_order.empty());

    size_t step = 0;
    for (auto& runnables : node_order) {
        if (parallel_exe) {
            BuildNodesParallel(runnables, step++, nodes, functions, opt,
                               output_variables, report_box, built_pipeline);
        } else {
            BuildNodesNonParallel(runnables, nodes, functions, opt,
                                  output_variables, report_box, built_pipeline);
        }
    }

    // Add total time report maker to front and back.
    if (report_box != nullptr) {
        auto start = std::make_shared<std::chrono::system_clock::time_point>();

        built_pipeline->insert(std::begin(*built_pipeline), [start] {
            *start = std::chrono::system_clock::now();
        });

        built_pipeline->emplace_back([start, report_box] {
            (*report_box)[TotalTimeStr()].execution_time =
                    std::chrono::system_clock::now() - *start;
        });
    }

    return true;
}

bool FaseCore::build(bool parallel_exe, bool profile) {
    START_TRY("build");
    // check if rebuild is necessary.
    if (profile == is_profiling_built && version == built_version) {
        return true;
    }

    if (!CheckSubPipelineDependencies(*this)) {
        std::cerr << "recursive depending of sub pipelines is found."
                  << std::endl;
        return false;
    }

    // setup sub pipelines
    for (auto& pair : sub_pipeline_fbs) {
        std::get<1>(pair)->init(*this, sub_pipelines.at(std::get<0>(pair)));
    }

    auto& nodes = getCurrentPipeline().nodes;
    getCurrentPipeline().multi = parallel_exe;

    // set report box ptr
    auto p_reports = &report_box;
    if (!profile) {
        p_reports = nullptr;
    }

    if (!BuildPipeline(nodes, functions, parallel_exe, false, &built_pipeline,
                       &output_variables, p_reports)) {
        return false;
    }

    built_version = version;
    is_profiling_built = profile;
    END_TRY();

    return true;
}

}  // namespace fase
