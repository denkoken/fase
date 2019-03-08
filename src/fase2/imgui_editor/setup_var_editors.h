
#ifndef SETUP_VAR_EDITORS_H_20190308
#define SETUP_VAR_EDITORS_H_20190308

#include <functional>
#include <map>
#include <typeindex>

#include "../variable.h"

namespace fase {

void SetupDefaultVarEditors(
        std::map<std::type_index,
                 std::function<Variable(const char*, const Variable&)>>*
                var_editors);
}

#endif  // SETUP_VAR_EDITORS_H_20190308
