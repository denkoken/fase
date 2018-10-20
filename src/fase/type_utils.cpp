#include "type_utils.h"

#include <sstream>

#define ADD_TYPE_CHECKER(map, type) \
    map[#type] = [](const Variable& v) { return v.isSameType<type>(); }
#define ADD_DESERIALISER(map, type, conv_f) \
    map[#type] = genConverter<type>(conv_f);
#define ADD_SERIALISER(map, type)        \
    map[#type] = [](const Variable& v) { \
        std::stringstream sstream;       \
        sstream << *v.getReader<type>(); \
        return sstream.str();            \
    }

namespace fase {
namespace {
template <typename T>
std::function<void(Variable&, const std::string&)> genConverter(
        const std::function<T(const std::string&)>& conv_f) {
    return [conv_f](Variable& v, const std::string& str) {
        v.create<T>(conv_f(str));
    };
}

}  // namespace

void SetupTypeUtils(TypeUtils* type_utils) {
    // Type checkers
    ADD_TYPE_CHECKER(type_utils->checkers, bool);
    ADD_TYPE_CHECKER(type_utils->checkers, char);
    ADD_TYPE_CHECKER(type_utils->checkers, unsigned char);
    ADD_TYPE_CHECKER(type_utils->checkers, short);
    ADD_TYPE_CHECKER(type_utils->checkers, unsigned short);
    ADD_TYPE_CHECKER(type_utils->checkers, int);
    ADD_TYPE_CHECKER(type_utils->checkers, unsigned int);
    ADD_TYPE_CHECKER(type_utils->checkers, long);
    ADD_TYPE_CHECKER(type_utils->checkers, unsigned long);
    ADD_TYPE_CHECKER(type_utils->checkers, long long);
    ADD_TYPE_CHECKER(type_utils->checkers, unsigned long long);
    ADD_TYPE_CHECKER(type_utils->checkers, float);
    ADD_TYPE_CHECKER(type_utils->checkers, double);
    ADD_TYPE_CHECKER(type_utils->checkers, long double);

    ADD_TYPE_CHECKER(type_utils->checkers, std::string);

    // Serializers
    ADD_SERIALISER(type_utils->serializers, bool);
    ADD_SERIALISER(type_utils->serializers, char);
    ADD_SERIALISER(type_utils->serializers, unsigned char);
    ADD_SERIALISER(type_utils->serializers, short);
    ADD_SERIALISER(type_utils->serializers, unsigned short);
    ADD_SERIALISER(type_utils->serializers, int);
    ADD_SERIALISER(type_utils->serializers, unsigned int);
    ADD_SERIALISER(type_utils->serializers, long);
    ADD_SERIALISER(type_utils->serializers, unsigned long);
    ADD_SERIALISER(type_utils->serializers, long long);
    ADD_SERIALISER(type_utils->serializers, unsigned long long);
    ADD_SERIALISER(type_utils->serializers, float);
    ADD_SERIALISER(type_utils->serializers, double);
    ADD_SERIALISER(type_utils->serializers, long double);

    ADD_SERIALISER(type_utils->serializers, std::string);

    // Deserializers
    using S = const std::string&;
    std::map<std::string, TypeUtils::Deserializer>& ss =
            type_utils->deserializers;
    ADD_DESERIALISER(ss, bool, [](S s) { return std::stoi(s); });
    ADD_DESERIALISER(ss, char, [](S s) { return *s.c_str(); });
    ADD_DESERIALISER(ss, unsigned char, [](S s) { return std::stoi(s); });
    ADD_DESERIALISER(ss, short, [](S s) { return std::stoi(s); });
    ADD_DESERIALISER(ss, unsigned short, [](S s) { return std::stoi(s); });
    ADD_DESERIALISER(ss, int, [](S s) { return std::stoi(s); });
    ADD_DESERIALISER(ss, unsigned int, [](S s) { return std::stol(s); });
    ADD_DESERIALISER(ss, long, [](S s) { return std::stol(s); });
    ADD_DESERIALISER(ss, unsigned long, [](S s) { return std::stoul(s); });
    ADD_DESERIALISER(ss, long long, [](S s) { return std::stoll(s); });
    ADD_DESERIALISER(ss, unsigned long long,
                     [](S s) { return std::stoull(s); });
    ADD_DESERIALISER(ss, float, [](S s) { return std::stof(s); });
    ADD_DESERIALISER(ss, double, [](S s) { return std::stod(s); });
    ADD_DESERIALISER(ss, long double, [](S s) { return std::stold(s); });

    ADD_DESERIALISER(ss, std::string, [](S s) { return s; });

    // DefMaker
    ADD_SERIALISER(type_utils->def_makers, bool);
    ADD_SERIALISER(type_utils->def_makers, char);
    ADD_SERIALISER(type_utils->def_makers, unsigned char);
    ADD_SERIALISER(type_utils->def_makers, short);
    ADD_SERIALISER(type_utils->def_makers, unsigned short);
    ADD_SERIALISER(type_utils->def_makers, int);
    ADD_SERIALISER(type_utils->def_makers, unsigned int);
    ADD_SERIALISER(type_utils->def_makers, long);
    ADD_SERIALISER(type_utils->def_makers, unsigned long);
    ADD_SERIALISER(type_utils->def_makers, long long);
    ADD_SERIALISER(type_utils->def_makers, unsigned long long);
    ADD_SERIALISER(type_utils->def_makers, float);
    ADD_SERIALISER(type_utils->def_makers, double);
    ADD_SERIALISER(type_utils->def_makers, long double);

    type_utils->def_makers["std::string"] = [](const Variable& v) {
        return "\"" + *v.getReader<std::string>() + "\"";
    };
}

}  // namespace fase
