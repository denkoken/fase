
#ifndef CORE_UTIL_H_20180824
#define CORE_UTIL_H_20180824

#include <string>

#include "core.h"

namespace fase {

std::string GenNativeCode(const FaseCore& core,
                          const std::string& entry_name = "Pipeline",
                          const std::string& indent = "    ");

std::string CoreToString(const FaseCore& core);

bool StringToCore(const std::string& str, FaseCore* core);

bool SaveFaseCore(const std::string& filename, const FaseCore& core);

bool LoadFaseCore(const std::string& filename, FaseCore* core);

std::vector<std::set<std::string>> GetCallOrder(
        const std::map<std::string, Node>& nodes);

}  // namespace fase

#endif  // CORE_UTIL_H_20180824
