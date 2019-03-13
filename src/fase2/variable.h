
#ifndef VARIABLE_H_20190206
#define VARIABLE_H_20190206

#include <functional>
#include <memory>
#include <string>
#include <typeindex>

#include "exceptions.h"

namespace fase {

class Variable {
public:
    Variable() : cloner([](auto&, auto&) {}), copyer([](auto&, auto&) {}) {}

    template <typename T>
    Variable(std::shared_ptr<T>&& p) {
        set(std::move(p));
    }

    Variable(const std::type_index& type_) : type(type_) {}

    Variable(Variable&&) = default;
    Variable& operator=(Variable&&) = default;

    Variable& operator=(Variable& v) {
        if (v.cloner != nullptr) {
            v.cloner(*this, v);
        }
        return *this;
    }

    Variable(const Variable& v) {
        if (v.cloner != nullptr) {
            v.cloner(*this, v);
        }
    }

    Variable& operator=(const Variable& v) {
        if (v.cloner != nullptr) {
            v.cloner(*this, v);
        }
        return *this;
    }

    Variable(Variable& v) {
        if (v.cloner != nullptr) {
            v.cloner(*this, v);
        }
    }

    ~Variable() = default;

    template <typename T, typename... Args>
    void create(Args&&... args) {
        set(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    void set(std::shared_ptr<T>&& v) {
        data = std::move(v);
        type = typeid(T);
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
        } else if (!*this) {
            throw(TryToGetEmptyVariable{});
        }
        return std::static_pointer_cast<T>(data);
    }

    template <typename T>
    std::shared_ptr<const T> getReader() const {
        if (!isSameType<T>() || !*this) {
            throw(WrongTypeCast(typeid(T), type));
        } else if (!*this) {
            throw(TryToGetEmptyVariable{});
        }
        return std::static_pointer_cast<const T>(data);
    }

    void copyTo(Variable& v) const {
        if (v.copyer != nullptr) {
            copyer(v, *this);
        }
    }

    Variable clone() const {
        return *this;
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
        return bool(data);
    }

    const std::type_index& getType() const {
        return type;
    }

private:
    using VFunc = void (*)(Variable&, const Variable&);

    std::shared_ptr<void> data;
    std::type_index       type = typeid(void);
    VFunc                 cloner = nullptr;
    VFunc                 copyer = nullptr;
};

}  // namespace fase

#endif  // VARIABLE_H_20190206
