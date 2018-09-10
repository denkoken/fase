
#ifndef CORE_UTIL_H_20180824
#define CORE_UTIL_H_20180824

#include <string>

#include "core.h"

namespace fase {

std::string GenNativeCode(const FaseCore& core,
                          const std::string& entry_name = "Pipeline",
                          const std::string& indent = "    ");

std::string CoreToString(const FaseCore& core, bool val = false);

bool StringToCore(const std::string& str, FaseCore* core);

bool SaveFaseCore(const std::string& filename, const FaseCore& core);

bool LoadFaseCore(const std::string& filename, FaseCore* core);

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

inline std::string InputFuncStr() {
    return ReportHeaderStr() + std::string("__input_f");
}

inline std::string OutputFuncStr() {
    return ReportHeaderStr() + std::string("__output_f");
}

inline std::string InputNodeStr() {
    return ReportHeaderStr() + std::string("__input");
}

inline std::string OutputNodeStr() {
    return ReportHeaderStr() + std::string("__output");
}

}  // namespace fase

#endif  // CORE_UTIL_H_20180824
