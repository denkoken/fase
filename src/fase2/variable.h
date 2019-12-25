
#ifndef VARIABLE_H_20190206
#define VARIABLE_H_20190206

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "debug_macros.h"
#include "exceptions.h"

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
    Variable() : member(std::make_shared<Substance>()) {}

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

    Variable& operator=(Variable& v) {
        free_if_not_managed_object();
        member = std::make_shared<Substance>();
        v.member->cloner(*this, v);
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

    Variable(Variable& v) : member(std::make_shared<Substance>()) {
        v.member->cloner(*this, v);
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

    void free() {
        member->data.reset();
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
            FASE_DEBUG_LOC_LOG(loc, "getWriter::WrongTypeCast");
            throw(WrongTypeCast(typeid(T), member->type));
        } else if (!*this) {
            std::string err_m = "getWriter::TryToGetEmptyVariable";
#ifdef FASE_IS_DEBUG_SOURCE_LOCATION_ON
            err_m += std::string(" called at ") + loc.file_name() + ":" +
                     std::to_string(loc.line());
#endif
            throw(TryToGetEmptyVariable(err_m));
        }
        return std::static_pointer_cast<T>(member->data);
    }

    template <typename T>
    std::shared_ptr<const T> getReader(FASE_DEBUG_LOC(loc)) const {
        if (!isSameType<T>()) {
            FASE_DEBUG_LOC_LOG(loc, "getReader::WrongTypeCast");
            throw(WrongTypeCast(typeid(T), member->type));
        } else if (!*this) {
            std::string err_m = "getReader::TryToGetEmptyVariable";
#ifdef FASE_IS_DEBUG_SOURCE_LOCATION_ON
            err_m += std::string(" called at ") + loc.file_name() + ":" +
                     std::to_string(loc.line());
#endif
            throw(TryToGetEmptyVariable(err_m));
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

    void toEmpty(const std::type_index& type) {
        member->type = type;
        member->cloner = [](Variable& d, const Variable& s) {
            d.member->type = s.member->type;
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
inline void Assign(std::vector<Variable>& c, Head&& h, Tail&&... tail) {
    c.reserve(c.size() + sizeof...(Tail) + 1);
    c.emplace_back(h);
    if constexpr (sizeof...(Tail) > 0) {
        Assign(c, tail...);
    }
}

inline void RefCopy(std::vector<Variable>& src, std::vector<Variable>* dst) {
    dst->clear();
    dst->resize(src.size());
    for (size_t i = 0; i < src.size(); i++) {
        (*dst)[i] = src[i].ref();
    }
}

inline void RefCopy(std::vector<Variable>::iterator&& begin,
                    std::vector<Variable>::iterator&& end,
                    std::vector<Variable>*            dst) {
    dst->clear();
    dst->reserve(std::size_t(end - begin));
    for (auto it = begin; it != end; it++) {
        dst->emplace_back(it->ref());
    }
}

} // namespace fase

#endif // VARIABLE_H_20190206
