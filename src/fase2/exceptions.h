
#ifndef EXCEPTIONS_H_20190313
#define EXCEPTIONS_H_20190313

#include <stdexcept>
#include <string>
#include <typeindex>

namespace fase {

class WrongTypeCast : public std::logic_error {
public:
    WrongTypeCast(std::type_index cast_type_, std::type_index casted_type_)
        : std::logic_error("WrongTypeCast"), cast_type(cast_type_),
          casted_type(casted_type_),
          err_message(std::string("fase::Variable : Invalid cast (") +
                      cast_type_.name() + std::string(" vs ") +
                      casted_type_.name() + std::string(")")) {}

    ~WrongTypeCast() {}

    std::type_index cast_type;
    std::type_index casted_type;

    const char* what() const noexcept {
        return err_message.c_str();
    }

private:
    std::string err_message;
};

class TryToGetEmptyVariable {};

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
