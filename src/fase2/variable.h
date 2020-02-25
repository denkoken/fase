
#ifndef VARIABLE_H_20190206
#define VARIABLE_H_20190206

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>

#include "debug_macros.h"
#include "exceptions.h"

#ifdef FASE_IS_DEBUG_SOURCE_LOCATION_ON
#define EXEPTION_STR(err_name, loc)                                            \
    [&](std::string f_name) {                                                  \
        return f_name + " throw " + err_name + " @ " + __FILE__ + ":" +        \
               std::to_string(__LINE__) + std::string(" called by ") +         \
               loc.file_name() + ":" + std::to_string(loc.line());             \
    }(__func__)
#else
#define EXEPTION_STR(err_name, loc)                                            \
    [&](std::string f_name) {                                                  \
        return f_name + " throw " + err_name + " @ " + __FILE__ + ":" +        \
               std::to_string(__LINE__);                                       \
    }(__func__)
#endif

namespace fase {

template <bool b>
struct CheckerForSFINAE {};

template <>
struct CheckerForSFINAE<true> {
    static constexpr bool value = true;
};
template <>
struct CheckerForSFINAE<false> {};

class Variable {
public:
    Variable() : member(std::make_shared<Substance>()) {
        toEmpty(typeid(void));
    }

    template <typename T>
    Variable(std::unique_ptr<T>&& p) : member(std::make_shared<Substance>()) {
        set<T>(std::move(p));
    }

    Variable(const std::type_index& type)
        : member(std::make_shared<Substance>()) {
        toEmpty(type);
    }

    template <typename T, bool b = CheckerForSFINAE<!std::is_same_v<
                                  T, const std::type_info>>::value>
    explicit Variable(T* object_ptr)
        : member(std::make_shared<Substance>()), is_managed_object(false) {
        set(std::shared_ptr<T>(object_ptr, [](auto&) {}));
    }

    Variable(Variable&& a) : member(std::move(a.member)) {
        is_managed_object = a.is_managed_object;
        a.is_managed_object = true;
    }
    Variable& operator=(Variable&& a) {
        free_if_not_managed_object();
        member = std::move(a.member);
        is_managed_object = a.is_managed_object;
        a.is_managed_object = true;
        return *this;
    }

    Variable(const Variable& v) : member(std::make_shared<Substance>()) {
        v.member->cloner(*this, v);
    }

    Variable& operator=(const Variable& v) {
        free_if_not_managed_object();
        member = std::make_shared<Substance>();
        v.member->cloner(*this, v);
        return *this;
    }

    ~Variable() {
        free_if_not_managed_object();
    }

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
    void assignedAs(T&& v FASE_COMMA_DEBUG_LOC(loc)) {
        using Type = std::decay_t<T>;
        if (bool(*this)) {
            *getWriter<Type>() = std::forward<T>(v);
        } else if (member->type == typeid(Type) ||
                   member->type == typeid(void)) {
            create<Type>(std::forward<T>(v));
        } else {
            throw(WrongTypeCast(typeid(Type), member->type,
                                EXEPTION_STR("WrongTypeCast", loc)));
        }
    }

    void free() {
        toEmpty(member->type);
        is_managed_object = true;
    }

    template <typename T>
    bool isSameType() const {
        return member->type == typeid(T);
    }

    bool isSameType(const Variable& v) const {
        return member->type == v.member->type;
    }

    template <typename T>
    std::shared_ptr<T> getWriter(FASE_DEBUG_LOC(loc)) {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), member->type,
                                EXEPTION_STR("WrongTypeCast", loc)));
        } else if (!*this) {
            throw(TryToGetEmptyVariable(
                    EXEPTION_STR("TryToGetEmptyVariable", loc)));
        }
        return std::static_pointer_cast<T>(member->data);
    }

    template <typename T>
    std::shared_ptr<const T> getReader(FASE_DEBUG_LOC(loc)) const {
        if (!isSameType<T>()) {
            throw(WrongTypeCast(typeid(T), member->type,
                                EXEPTION_STR("WrongTypeCast", loc)));
        } else if (!*this) {
            throw(TryToGetEmptyVariable(
                    EXEPTION_STR("TryToGetEmptyVariable", loc)));
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
        VFunc                 cloner = [](auto&, auto&) { assert(false); };
        VFunc                 copyer = [](auto&, auto&) { assert(false); };
    };

    explicit Variable(std::shared_ptr<Substance>& m) : member(m) {}

    void toEmpty(const std::type_index& type) {
        member->data.reset();
        member->type = type;
        member->cloner = [](Variable& d, const Variable& s) {
            d.toEmpty(s.member->type);
        };
        member->copyer = [](Variable& d, const Variable& s) {
            if (d.getType() == s.getType()) {
                s.member->cloner(d, s);
            } else {
                throw(WrongTypeCast(d.member->type, s.member->type));
            }
        };
    }
    void free_if_not_managed_object() {
        if (!is_managed_object) {
            free();
        }
    }

    std::shared_ptr<Substance> member;
    bool                       is_managed_object = true;
};

template <typename Head, typename... Tail>
inline void Assign(std::deque<Variable>& c, Head&& h, Tail&&... tail) {
    c.emplace_back(h);
    if constexpr (sizeof...(Tail) > 0) {
        Assign(c, tail...);
    }
}

inline void RefCopy(std::deque<Variable>::iterator&& begin,
                    std::deque<Variable>::iterator&& end,
                    std::deque<Variable>*            dst) {
    dst->clear();
    for (auto it = begin; it != end; it++) {
        dst->emplace_back(it->ref());
    }
}

inline void RefCopy(std::deque<Variable>& src, std::deque<Variable>* dst) {
    RefCopy(src.begin(), src.end(), dst);
}

} // namespace fase

#undef EXEPTION_STR

#endif // VARIABLE_H_20190206
