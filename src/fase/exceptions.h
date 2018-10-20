#ifndef FASE_EXCEPTIONS_H_20180617
#define FASE_EXCEPTIONS_H_20180617

#include <stdexcept>
#include <string>
#include <typeinfo>

namespace fase {

class WrongTypeCast : public std::logic_error {
public:
    WrongTypeCast(const std::type_info& cast_type_,
                  const std::type_info& casted_type_)
        : std::logic_error("WrongTypeCast"),
          cast_type(&cast_type_),
          casted_type(&casted_type_),
          err_message(
                  std::string("Variable::getReader() failed : Invalid cast (") +
                  cast_type_.name() + std::string(" vs ") +
                  casted_type_.name() + std::string(")")) {}

    ~WrongTypeCast() {}

    const std::type_info* cast_type;
    const std::type_info* casted_type;

    const char* what() const noexcept {
        return err_message.c_str();
    }

private:
    std::string err_message;
};

class InvalidArgN : public std::logic_error {
public:
    InvalidArgN(const size_t& expected_n_, const size_t& input_n_)
        : std::logic_error("InvalidArgN"),
          input_n(input_n_),
          expected_n(expected_n_),
          err_message(std::string("FunctionBuilder::build() failed : Invalid "
                                  "Number of Arguments ( expect ") +
                      std::to_string(expected_n_) +
                      std::string(" , but # of arguments is ") +
                      std::to_string(input_n_) + std::string(")")) {}

    const size_t input_n;
    const size_t expected_n;

    const char* what() const noexcept {
        return err_message.c_str();
    }

private:
    std::string err_message;
};

// TODO change to be extended std::nested_exception
class ErrorThrownByNode : public std::runtime_error {
public:
    ErrorThrownByNode(const std::string& node_name_,
                      const std::string& err_message_)
        : runtime_error("ErrorThrownByNode"),
          node_name(node_name_),
          err_message(err_message_) {}

    std::string node_name;
    std::string err_message;

    const char* what() const noexcept {
        return ("[" + node_name + "] " + err_message).c_str();
    }
};

}  // namespace fase

#endif  // FASE_EXCEPTIONS_H_20180617
