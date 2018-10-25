#include "type_utils.h"

#include <sstream>

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
#define ADD_TYPE_CHECKER(type)                            \
    type_utils->checkers[#type] = [](const Variable& v) { \
        return v.isSameType<type>();                      \
    }
#define ADD_SERIALISER(type)                                 \
    type_utils->serializers[#type] = [](const Variable& v) { \
        std::stringstream sstream;                           \
        sstream << *v.getReader<type>();                     \
        return sstream.str();                                \
    }
#define ADD_DESERIALISER(type, conv_f) \
    type_utils->deserializers[#type] = \
            genConverter<type>([](const std::string& s) conv_f);
#define ADD_DEF_MAKER(type)                                 \
    type_utils->def_makers[#type] = [](const Variable& v) { \
        std::stringstream sstream;                          \
        sstream << *v.getReader<type>();                    \
        return sstream.str();                               \
    }
#define ADD_NAME(type) type_utils->names[&typeid(type)] = #type

#define ADD_ALL(type, conv_f)       \
    ADD_TYPE_CHECKER(type);         \
    ADD_SERIALISER(type);           \
    ADD_DESERIALISER(type, conv_f); \
    ADD_DEF_MAKER(type);            \
    ADD_NAME(type);

    // clang-format off
    ADD_ALL(bool,               { return std::stoi(s);   });
    ADD_ALL(bool,               { return std::stoi(s);   });
    ADD_ALL(char,               { return *s.c_str();     });
    ADD_ALL(unsigned char,      { return std::stoi(s);   });
    ADD_ALL(short,              { return std::stoi(s);   });
    ADD_ALL(unsigned short,     { return std::stoi(s);   });
    ADD_ALL(int,                { return std::stoi(s);   });
    ADD_ALL(unsigned int,       { return std::stol(s);   });
    ADD_ALL(long,               { return std::stol(s);   });
    ADD_ALL(unsigned long,      { return std::stoul(s);  });
    ADD_ALL(long long,          { return std::stoll(s);  });
    ADD_ALL(unsigned long long, { return std::stoull(s); });
    ADD_ALL(float,              { return std::stof(s);   });
    ADD_ALL(double,             { return std::stod(s);   });
    ADD_ALL(long double,        { return std::stold(s);  });

    // clang-format on

    ADD_TYPE_CHECKER(std::string);
    ADD_SERIALISER(std::string);
    ADD_DESERIALISER(std::string, { return s; });  // TODO fix
    type_utils->def_makers["std::string"] = [](const Variable& v) {
        return "\"" + *v.getReader<std::string>() + "\"";
    };
    ADD_NAME(std::string);

#undef ADD_TYPE_CHECKER
#undef ADD_SERIALISER
#undef ADD_DESERIALISER
#undef ADD_DEF_MAKER
#undef ADD_NAME
}

}  // namespace fase
