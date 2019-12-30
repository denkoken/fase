
#ifndef EXCEPTIONS_H_20190313
#define EXCEPTIONS_H_20190313

#include <stdexcept>
#include <string>
#include <typeindex>

#include "utils.h"

namespace fase {

class WrongTypeCast : public std::logic_error {
public:
    WrongTypeCast(std::type_index cast_type_, std::type_index casted_type_,
                  std::string m = {})
        : std::logic_error("WrongTypeCast"), cast_type(cast_type_),
          casted_type(casted_type_),
          err_message(m + std::string(" Invalid cast (") +
                      type_name(cast_type_) + std::string(" vs ") +
                      type_name(casted_type_) + std::string(")")) {}

    ~WrongTypeCast() {}

    std::type_index cast_type;
    std::type_index casted_type;

    const char* what() const noexcept {
        return err_message.c_str();
    }

private:
    std::string err_message;
};

class TryToGetEmptyVariable : public std::runtime_error {
public:
    TryToGetEmptyVariable(const std::string& m) : std::runtime_error(m) {}
};

class ErrorThrownByNode : public std::nested_exception {
public:
    ErrorThrownByNode(const std::string& node_name_)
        : std::nested_exception(), node_name(node_name_) {}

    std::string node_name;

    const char* what() const noexcept {
        return ("[" + node_name + "] throws error").c_str();
    }
};

} // namespace fase

#endif // EXCEPTIONS_H_20190313
