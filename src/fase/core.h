#ifndef FASE_CORE_H_20180622
#define FASE_CORE_H_20180622

#include <algorithm>
#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "binded_pipe.h"
#include "function_node.h"
#include "variable.h"

namespace fase {
struct Link {
    std::string node_name;
    size_t arg_idx;
};

struct Node {
    std::string func_repr;    // Corresponding to function representation
    std::vector<Link> links;  // size == |args|
    std::vector<std::tuple<size_t, Link>> rev_links;
    std::vector<std::string> arg_reprs;  // size == |args|
    std::vector<Variable> arg_values;    // size == |args|
    int priority;
};

struct Function {
    std::shared_ptr<FunctionBuilderBase> builder;
    std::vector<std::string> default_arg_reprs;    // size == |args|
    std::vector<std::string> arg_names;            // size == |args|
    std::vector<Variable> default_arg_values;      // size == |args|
    std::vector<const std::type_info*> arg_types;  // size == |args| (cleaned)
    std::vector<bool> is_input_args;               // size == |args|
    std::string code;
};

struct Pipeline {
    std::map<std::string, Node> nodes;  // [name, Node]
    bool multi = false;
};

class FaseCore {
public:
    FaseCore();

    FaseCore(FaseCore&);
    FaseCore(const FaseCore&) = delete;
    FaseCore(FaseCore&&) = default;
    FaseCore& operator=(FaseCore&);
    FaseCore& operator=(const FaseCore&) = delete;
    FaseCore& operator=(FaseCore&&) = default;

    // ## Setup ##
    template <typename Ret, typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<Ret(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names = {},
            const std::array<Variable, sizeof...(Args)>& default_args = {},
            const std::string& code = "No data.");

    // ## Editing nodes ##
    bool addNode(const std::string& node_name, const std::string& func_repr,
                 const int& priority = 0);

    bool delNode(const std::string& node_name) noexcept;

    bool renameNode(const std::string& old_name, const std::string& new_name);

    bool setPriority(const std::string& node_name, const int& priority);

    void clearNodeArg(const std::string& node_name, const size_t arg_idx);

    bool setNodeArg(const std::string& node_name, const size_t arg_idx,
                    const std::string& arg_repr, const Variable& arg_val);

    // ## Editing links ##
    bool addLink(const std::string& src_node_name, const size_t& src_arg_idx,
                 const std::string& dst_node_name, const size_t& dst_arg_idx);
    void delLink(const std::string& dst_node_name, const size_t& dst_arg_idx);

    // ## Editing Input/Output node ##
    bool addInput(const std::string& name, const std::type_info* type = nullptr,
                  const Variable& default_value = {},
                  const std::string& default_arg_repr = "");
    bool delInput(const size_t& idx);

    bool addOutput(const std::string& name,
                   const std::type_info* type = nullptr,
                   const Variable& default_value = {},
                   const std::string& default_arg_repr = "");
    bool delOutput(const size_t& idx);

    void lockInOut();
    void unlockInOut();

    void switchPipeline(const std::string& pipeline_name);
    void renamePipeline(const std::string& pipeline_name) noexcept;
    void deletePipeline(const std::string& pipeline_name) noexcept;

    bool makeSubPipeline(const std::string& name);

    // ## Building, Running ##
    bool build(bool parallel_exe = false, bool profile = false);
    const std::map<std::string, ResultReport>& run();

    template <typename... Args>
    void setInput(Args&&... args);

    // ## Getters ##
    const std::map<std::string, Function>& getFunctions() const;
    const std::map<std::string, Node>& getNodes() const;
    std::map<std::string, const Pipeline*> getPipelines() const;

    const std::string& getCurrentPipelineName() const noexcept;
    std::vector<std::string> getPipelineNames() const;
    std::vector<std::string> getSubPipelineNames() const;
    const std::string& getMainPipelineNameLastSelected() const noexcept;

    const std::vector<Variable>& getOutputs() const;
    template <typename T>
    T getOutput(const std::string& node_name, const size_t& arg_idx,
                const T& default_value = T());

    /// Get FaseCore version; use this for check nodes have been updated.
    int getVersion() const;

private:
    const static char MainPipeInOutName[];

    // Registered functions
    std::map<std::string, Function> functions;  // [repr, Function]

    std::map<std::string, Pipeline> pipelines;
    std::string current_pipeline;
    std::string main_pipeline_last_selected;

    // ## Sub Pipelines ##
    std::map<std::string, Pipeline> sub_pipelines;
    std::map<std::string, std::shared_ptr<BindedPipeline>>
            sub_pipeline_fbs;  /// function builders

    int version;

    bool is_locked_inout = false;  /// about not sub pipelines inout

    // Built pipeline
    std::vector<std::function<void()>> built_pipeline;
    std::map<std::string, std::vector<Variable>> output_variables;
    int built_version;
    bool is_profiling_built;

    std::map<std::string, ResultReport> report_box;

    // ## Utility functions. ##
    bool checkNodeName(const std::string& name);
    Pipeline& getCurrentPipeline();
    const Pipeline& getCurrentPipeline() const;

    std::string getEdittingInputFunc();
    std::string getEdittingOutputFunc();
    void initInOut();
};

}  // namespace fase

#include "core_impl.h"

#endif  // CORE_H_20180622
