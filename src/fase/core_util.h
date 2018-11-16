#ifndef CORE_UTIL_H_20180824
#define CORE_UTIL_H_20180824

#include <set>
#include <string>
#include <vector>

#include "type_utils.h"

namespace fase {

struct Link;
struct Node;
struct Function;
struct ResultReport;
class FaseCore;

std::string GenNativeCode(const FaseCore& core, const TypeUtils& utils,
                          const std::string& entry_name = "Pipeline",
                          const std::string& indent = "    ") noexcept;

std::string CoreToString(const FaseCore& core, const TypeUtils& utils);

bool StringToCore(const std::string& str, FaseCore* core,
                  const TypeUtils& utils);

bool SaveFaseCore(const std::string& filename, const FaseCore& core,
                  const TypeUtils& utils);

bool LoadFaseCore(const std::string& filename, FaseCore* core,
                  const TypeUtils& utils, const std::string& target_name = "");

bool ImportSubPipeline(const std::string& filename, FaseCore* core,
                       const TypeUtils&   utils,
                       const std::string& target_name = "");

std::vector<std::vector<Link>> getReverseLinks(
        const std::string& node, const std::map<std::string, Node>& nodes);

/// if a empty vector returned, something went wrong.
std::vector<std::set<std::string>> GetCallOrder(
        const std::map<std::string, Node>& nodes);

bool BuildPipeline(
        const std::map<std::string, Node>&     nodes,
        const std::map<std::string, Function>& functions, bool parallel_exe,
        std::vector<std::function<void()>>*           built_pipeline,
        std::map<std::string, std::vector<Variable>>* output_variables,
        std::map<std::string, ResultReport>*          report_box);

inline std::string ReportHeaderStr() {
    return std::string("FASE:");
}

static constexpr char SUB_PIPE_STR[] = "__sub_pipe_";
static constexpr char INPUT_F_HEADER_STR[] = "__input_f_";
static constexpr char OUTPUT_F_HEADER_STR[] = "__output_f_";

inline std::string StepStr(const size_t& step) {
    return ReportHeaderStr() + std::string("__step") + std::to_string(step);
}

inline std::string TotalTimeStr() {
    return ReportHeaderStr() + std::string("__tatal_time");
}

inline std::string InputFuncStr(const std::string& pipeline_name) {
    return ReportHeaderStr() + INPUT_F_HEADER_STR + pipeline_name;
}

inline std::string OutputFuncStr(const std::string& pipeline_name) {
    return ReportHeaderStr() + OUTPUT_F_HEADER_STR + pipeline_name;
}

inline std::string SubPipelineFuncStr(const std::string& pipeline_name) {
    return ReportHeaderStr() + std::string(SUB_PIPE_STR) + pipeline_name;
}
inline bool IsSubPipelineFuncStr(const std::string& name) {
    return name.find(ReportHeaderStr() + std::string(SUB_PIPE_STR)) !=
           std::string::npos;
}
inline std::string GetSubPipelineNameFromFuncStr(const std::string& name) {
    return {std::begin(name) +
                    long((ReportHeaderStr() + std::string(SUB_PIPE_STR))
                                 .size()),
            std::end(name)};
}

inline bool IsInputFuncStr(const std::string& name) {
    return name.find(ReportHeaderStr() + INPUT_F_HEADER_STR) !=
           std::string::npos;
}

inline bool IsOutputFuncStr(const std::string& name) {
    return name.find(ReportHeaderStr() + OUTPUT_F_HEADER_STR) !=
           std::string::npos;
}

inline std::string InputNodeStr() {
    return ReportHeaderStr() + std::string("__input");
}

inline std::string OutputNodeStr() {
    return ReportHeaderStr() + std::string("__output");
}

inline bool IsSpecialNodeName(const std::string& name) {
    return name.find(ReportHeaderStr()) != std::string::npos;
}

inline bool IsSpecialFuncName(const std::string& name) {
    return name.find(ReportHeaderStr()) != std::string::npos;
}

}  // namespace fase

#endif  // CORE_UTIL_H_20180824
