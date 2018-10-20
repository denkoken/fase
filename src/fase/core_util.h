#ifndef CORE_UTIL_H_20180824
#define CORE_UTIL_H_20180824

#include <string>

#include "core.h"
#include "type_utils.h"

namespace fase {
std::string GenNativeCode(const FaseCore& core, const TypeUtils& utils,
                          const std::string& entry_name = "Pipeline",
                          const std::string& indent = "    ");

std::string CoreToString(const FaseCore& core, const TypeUtils& utils);

bool StringToCore(const std::string& str, FaseCore* core,
                  const TypeUtils& utils);

bool SaveFaseCore(const std::string& filename, const FaseCore& core,
                  const TypeUtils& utils);

bool LoadFaseCore(const std::string& filename, FaseCore* core,
                  const TypeUtils& utils);

std::vector<std::vector<Link>> getReverseLinks(
        const std::string& node, const std::map<std::string, Node>& nodes);

/// if a empty vector returned, something went wrong.
std::vector<std::set<std::string>> GetCallOrder(
        const std::map<std::string, Node>& nodes);

inline std::string ReportHeaderStr() {
    return std::string("FASE:");
}

inline std::string StepStr(const size_t& step) {
    return ReportHeaderStr() + std::string("__step") + std::to_string(step);
}

inline std::string TotalTimeStr() {
    return ReportHeaderStr() + std::string("__tatal_time");
}

inline std::string InputFuncStr(const std::string& project_name) {
    return ReportHeaderStr() + std::string("__input_f_") + project_name;
}

inline std::string OutputFuncStr(const std::string& project_name) {
    return ReportHeaderStr() + std::string("__output_f_") + project_name;
}

inline bool IsInputFuncStr(const std::string& name) {
    return name.find(ReportHeaderStr() + std::string("__input_f_")) !=
           std::string::npos;
}

inline bool IsOutputFuncStr(const std::string& name) {
    return name.find(ReportHeaderStr() + std::string("__output_f_")) !=
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
