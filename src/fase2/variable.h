
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
    Variable() : member(std::make_shared<Substance>()) {}

    template <typename T>
    Variable(std::unique_ptr<T>&& p) : member(std::make_shared<Substance>()) {
        set<T>(std::move(p));
    }

    Variable(const std::type_index& type)
        : member(std::make_shared<Substance>()) {
        member->type = type;
        member->copyer = [](Variable& d, const Variable& s) {
            if (d.getType() == s.getType()) {
                s.member->cloner(d, s);
            } else {
                throw(WrongTypeCast(d.member->type, s.member->type));
            }
        };
    }

    Variable(Variable&&) = default;
    Variable& operator=(Variable&&) = default;

    Variable& operator=(Variable& v) {
        member = std::make_shared<Substance>();
        v.member->cloner(*this, v);
        return *this;
    }

    Variable(const Variable& v) : member(std::make_shared<Substance>()) {
        v.member->cloner(*this, v);
    }

    Variable& operator=(const Variable& v) {
        member = std::make_shared<Substance>();
        v.member->cloner(*this, v);
        return *this;
    }

    Variable(Variable& v) : member(std::make_shared<Substance>()) {
        v.member->cloner(*this, v);
    }

    ~Variable() = default;

    template <typename T, typename... Args>
    void create(Args&&... args) {
        set(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    void set(std::shared_ptr<T>&& v) {
        member->data = std::move(v);
        member->type = typeid(T);
        member->cloner = [](Variable& d, const Variable& s) {
            d.create<T>(*s.getReader<T>());
        };
        member->copyer = [](Variable& d, const Variable& s) {
            *d.getWriter<T>() = *s.getReader<T>();
        };
    }

    template <typename T>
    bool isSameType() const {
        return member->type == typeid(T);
    }

    bool isSameType(const Variable& v) const {
        return member->type == v.member->type;
    }

    template <typename T>
    std::shared_ptr<T> getWriter() {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), member->type));
        } else if (!*this) {
            throw(TryToGetEmptyVariable{});
        }
        return std::static_pointer_cast<T>(member->data);
    }

    template <typename T>
    std::shared_ptr<const T> getReader() const {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), member->type));
        } else if (!*this) {
            throw(TryToGetEmptyVariable{});
        }
        return std::static_pointer_cast<const T>(member->data);
    }

    void copyTo(Variable& v) const {
        v.member->copyer(v, *this);
    }

    Variable clone() const {
        return *this;
    }

    Variable ref() {
        return Variable(member);
    }

    explicit operator bool() const noexcept {
        return bool(member->data);
    }

    const std::type_index& getType() const {
        return member->type;
    }

private:
    using VFunc = void (*)(Variable&, const Variable&);
    struct Substance {
        std::shared_ptr<void> data;
        std::type_index       type = typeid(void);
        VFunc                 cloner = [](auto&, auto&) {};
        VFunc                 copyer = [](auto&, auto&) {};
    };

    explicit Variable(std::shared_ptr<Substance>& m) : member(m) {}

    std::shared_ptr<Substance> member;
};

}  // namespace fase

#endif  // VARIABLE_H_20190206
