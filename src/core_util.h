
#ifndef CORE_UTIL_H_20180824
#define CORE_UTIL_H_20180824

#include <string>

#include "core.h"

namespace fase {

std::string genNativeCode(const FaseCore& core,
                          const std::string& entry_name = "Pipeline",
                          const std::string& indent = "    ");

}


#endif // CORE_UTIL_H_20180824
