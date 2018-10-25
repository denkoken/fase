#include "core.h"

#include <thread>

namespace fase {

namespace {

std::vector<Variable*> BindVariables(
        const Node& node,
        const std::map<std::string, std::vector<Variable>>& exists_variables,
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

}  // namespace

std::function<void()> FaseCore::buildNode(
        const std::string& node_name, const std::vector<Variable*>& args,
        std::map<std::string, ResultReport>* report_box_) const {
    const Function& func = functions.at(
            pipelines.at(editting_pipeline).nodes.at(node_name).func_repr);
    if (node_name == InputNodeStr() || node_name == OutputNodeStr()) {
        return [] {};
    }
    if (report_box_ != nullptr) {
        return wrapPipe(node_name,
                        func.builder->build(args, &(*report_box_)[node_name]));
    } else {
        return wrapPipe(node_name, func.builder->build(args));
    }
}

void FaseCore::buildNodesParallel(
        const std::set<std::string>& runnables,
        const size_t& step,  // for report.
        std::map<std::string, ResultReport>* report_box_) {
    auto& nodes = pipelines[editting_pipeline].nodes;
    std::map<std::string, std::vector<Variable*>> variable_ps;
    for (auto& runnable : runnables) {
        variable_ps[runnable] = BindVariables(nodes[runnable], output_variables,
                                              &output_variables[runnable]);
    }

    // Build
    std::vector<std::function<void()>> funcs;
    for (auto& runnable : runnables) {
        funcs.emplace_back(
                buildNode(runnable, variable_ps[runnable], report_box_));
    }
    if (report_box_ != nullptr) {
        built_pipeline.push_back([funcs, report_box_, step] {
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
        built_pipeline.push_back([funcs] {
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

void FaseCore::buildNodesNonParallel(
        const std::set<std::string>& runnables,
        std::map<std::string, ResultReport>* report_box_) {
    auto& nodes = pipelines[editting_pipeline].nodes;
    for (auto& runnable : runnables) {
        const Node& node = nodes[runnable];
        auto bound_variables = BindVariables(node, output_variables,
                                             &output_variables[runnable]);

        // Build
        built_pipeline.emplace_back(
                buildNode(runnable, bound_variables, report_box_));
    }
}

bool FaseCore::build(bool parallel_exe, bool profile) {
    if (profile == is_profiling_built && version == built_version) {
        return true;
    }

    // TODO change parallel execution system.

    auto& nodes = pipelines[editting_pipeline].nodes;
    pipelines[editting_pipeline].multi = parallel_exe;
    built_pipeline.clear();
    output_variables.clear();
    report_box.clear();

    // Build running order.
    auto node_order = GetCallOrder(nodes);
    assert(!node_order.empty());

    if (profile) {
        auto start = std::make_shared<std::chrono::system_clock::time_point>();
        built_pipeline.push_back(
                [start] { *start = std::chrono::system_clock::now(); });

        size_t step = 0;
        for (auto& runnables : node_order) {
            if (parallel_exe) {
                buildNodesParallel(runnables, step++, &report_box);
            } else {
                buildNodesNonParallel(runnables, &report_box);
            }
        }

        built_pipeline.push_back([start, &report_box = this->report_box] {
            report_box[TotalTimeStr()].execution_time =
                    std::chrono::system_clock::now() - *start;
        });
    } else {
        for (auto& runnables : node_order) {
            if (parallel_exe) {
                buildNodesParallel(runnables, 0, nullptr);
            } else {
                buildNodesNonParallel(runnables, nullptr);
            }
        }
    }

    built_version = version;
    is_profiling_built = profile;

    return true;
}

}  // namespace fase
