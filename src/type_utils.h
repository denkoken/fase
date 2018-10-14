#ifndef TYPE_UTILS_H_20180911
#define TYPE_UTILS_H_20180911

#include <map>

#include "variable.h"

namespace fase {

struct TypeUtils {
    using Serializer = std::function<std::string(const Variable&)>;
    using Deserializer = std::function<void(Variable&, const std::string&)>;
    using Checker = std::function<bool(const Variable&)>;
    using DefMaker = Serializer;

    std::map<std::string, Serializer> serializers;
    std::map<std::string, Deserializer> deserializers;
    std::map<std::string, Checker> checkers;
    std::map<std::string, DefMaker> def_makers;
    std::map<const std::type_info*, std::string> names;
};

void SetupTypeUtils(TypeUtils*);

}  // namespace fase

#endif  // TYPE_UTILS_H_20180911
