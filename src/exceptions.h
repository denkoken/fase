#ifndef FASE_EXCEPTIONS_H_20180617
#define FASE_EXCEPTIONS_H_20180617

#include <stdexcept>
#include <string>
#include <typeinfo>

namespace fase {

class WrongTypeCast : public std::logic_error {
public:
    WrongTypeCast(const std::type_info& cast_t, const std::type_info& casted_t)
        : std::logic_error("WrongTypeCast"),
          cast_type(&cast_t),
          casted_type(&casted_t),
          err_message(
                  std::string("Variable::getReader() failed : Invalid cast (") +
                  cast_type->name() + std::string(" vs ") +
                  casted_type->name() + std::string(")")) {}

    const std::type_info* cast_type;
    const std::type_info* casted_type;

    const char* what() const noexcept { return err_message.c_str(); }

private:
    std::string err_message;
};

class InvalidArgN : public std::logic_error {
public:
    InvalidArgN(const size_t& expect_n, const size_t& input_n)
        : std::logic_error("InvalidArgN"),
          input_n(input_n),
          expected_n(expect_n),
          err_message(std::string("FunctionBuilder::build() failed : Invalid "
                                  "Number of Arguments ( expect ") +
                      std::to_string(expected_n) +
                      std::string(" , but # of arguments is ") +
                      std::to_string(input_n) + std::string(")")) {}

    const size_t input_n;
    const size_t expected_n;

    const char* what() const noexcept { return err_message.c_str(); }

private:
    std::string err_message;
};

}  // namespace fase

#endif  // FASE_EXCEPTIONS_H_20180617
