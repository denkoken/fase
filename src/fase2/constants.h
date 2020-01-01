
#ifndef CONSTANTS_H_20190212
#define CONSTANTS_H_20190212

#include <string>

namespace fase {

constexpr char kInputFuncName[] = "FASE::InputFunc";
constexpr char kOutputFuncName[] = "FASE::OutputFunc";

constexpr char kReturnValueID[] = "[[returned]]";

static inline std::string InputNodeName() {
    return "Input";
}

static inline std::string OutputNodeName() {
    return "Output";
}
} // namespace fase

#endif // CONSTANTS_H_20190212
