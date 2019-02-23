
#ifndef VARIABLE_H_20190206
#define VARIABLE_H_20190206

#include <functional>
#include <memory>
#include <string>
#include <typeindex>

namespace fase {

class WrongTypeCast : public std::logic_error {
public:
    WrongTypeCast(std::type_index cast_type_, std::type_index casted_type_)
        : std::logic_error("WrongTypeCast"),
          cast_type(cast_type_),
          casted_type(casted_type_),
          err_message(
                  std::string("Variable::getReader() failed : Invalid cast (") +
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

class Variable {
public:
    Variable() : cloner([](auto&, auto&) {}), copyer([](auto&, auto&) {}) {}

    template <typename T>
    Variable(std::shared_ptr<T>&& p) {
        set(std::move(p));
    }

    Variable(Variable&&) = default;
    Variable& operator=(Variable&&) = default;

    Variable& operator=(Variable& v) {
        v.cloner(*this, v);
        return *this;
    }

    Variable(const Variable& v) {
        v.cloner(*this, v);
    }

    Variable& operator=(const Variable& v) {
        v.cloner(*this, v);
        return *this;
    }

    Variable(Variable& v) {
        v.cloner(*this, v);
    }

    ~Variable() = default;

    template <typename T, typename... Args>
    void create(Args&&... args) {
        set(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    void set(std::shared_ptr<T>&& v) {
        data = std::move(v);
        type = typeid(T);  // The lifetime extends to the end of the program.
        cloner = [](Variable& d, const Variable& s) {
            d.create<T>(*s.getReader<T>());
        };
        copyer = [](Variable& d, const Variable& s) {
            *d.getWriter<T>() = *s.getReader<T>();
        };
    }

    template <typename T>
    bool isSameType() const {
        return type == typeid(T);
    }

    bool isSameType(const Variable& v) const {
        return type == v.type;
    }

    template <typename T>
    std::shared_ptr<T> getWriter() {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), type));
        }
        return std::static_pointer_cast<T>(data);
    }

    template <typename T>
    std::shared_ptr<const T> getReader() const {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), type));
        }
        return std::static_pointer_cast<const T>(data);
    }

    void copyTo(Variable& v) const {
        copyer(v, *this);
    }

    Variable clone() const {
        Variable v;
        cloner(v, *this);
        return v;
    }

    Variable ref() {
        Variable v;
        v.data = data;
        v.type = type;
        v.cloner = cloner;
        v.copyer = copyer;
        return v;
    }

    explicit operator bool() const noexcept {
        return type != typeid(void);
    }

private:
    std::shared_ptr<void>                           data;
    std::type_index                                 type = typeid(void);
    std::function<void(Variable&, const Variable&)> cloner;
    std::function<void(Variable&, const Variable&)> copyer;
};

}  // namespace fase

#endif  // VARIABLE_H_20190206
