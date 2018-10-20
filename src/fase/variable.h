#ifndef FASE_VARIABLE_H_20180617
#define FASE_VARIABLE_H_20180617

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <typeinfo>

#include "exceptions.h"

namespace fase {

class Variable {
public:
    Variable() = default;

    template <typename T>
    Variable(T&& value) {
        using TypeCVR = T;
        using TypeCV = typename std::remove_reference<TypeCVR>::type;
        using Type = typename std::remove_cv<TypeCV>::type;
        create<Type>(std::forward<TypeCV>(value));
    }

    template <typename T>
    Variable(std::shared_ptr<T>& v) {
        set<T>(v);
    }

#if defined(_WIN64) || defined(_WIN32)
    template <>
    Variable(Variable&& v) {
        data = v.data;
        type = v.type;
        cloner = v.cloner;
    }

    template <>
    Variable(Variable& v) {
        data = v.data;
        type = v.type;
        cloner = v.cloner;
    }
#else
    Variable(Variable&) = default;
    Variable& operator=(Variable&) = default;
#endif
    Variable(const Variable&) = default;
    Variable& operator=(const Variable&) = default;
    Variable(Variable&&) = default;
    Variable& operator=(Variable&&) = default;

    ~Variable() = default;

    template <typename T, typename... Args>
    void create(Args&&... args) {
        set(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    void set(std::shared_ptr<T> v) {
        data = v;
        type = &typeid(T);  // The lifetime extends to the end of the program.
        cloner = [](Variable& d, const Variable& s) {
            d.create<T>(*s.getReader<T>());
        };
    }

    template <typename T>
    bool isSameType() const {
        return *type == typeid(T);
    }

    bool isSameType(const Variable& v) const {
        return *type == *v.type;
    }

    template <typename T>
    std::shared_ptr<T> getWriter() {
        if (!isSameType<T>()) {
            // Create by force
            create<T>();
        }
        return std::static_pointer_cast<T>(data);
    }

    template <typename T>
    std::shared_ptr<T> getReader() const {
        if (!isSameType<T>()) {
            // Invalid type cast
            throw(WrongTypeCast(typeid(T), *type));
        }
        return std::static_pointer_cast<T>(data);
    }

    void copy(Variable& v) const {
        cloner(v, *this);
    }

    Variable clone() const {
        Variable v;
        cloner(v, *this);
        return v;
    }

    explicit operator bool() const noexcept {
        return type != &typeid(void);
    }

private:
    std::shared_ptr<void> data;
    const std::type_info* type = &typeid(void);
    std::function<void(Variable&, const Variable&)> cloner;
};

}  // namespace fase

#endif  // FASE_VARIABLE_H_20180617