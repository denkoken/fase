#ifndef FASE_VARIABLE_H_20180617
#define FASE_VARIABLE_H_20180617

#include <cstring>
#include <memory>
#include <typeinfo>

#include "exceptions.h"

namespace fase {

class Variable {
public:
    Variable() = default;

    template <typename T>
    Variable(T &&value)
        : data(std::make_shared<typename std::remove_reference<T>::type>(
              std::forward<T>(value))),
          type(&typeid(typename std::remove_reference<T>::type)) {}

    Variable(Variable &) = default;
    Variable &operator=(Variable &) = default;
    Variable(const Variable &) = default;
    Variable &operator=(const Variable &) = default;
    Variable(Variable &&) = default;
    Variable &operator=(Variable &&) = default;

    ~Variable() = default;

    template <typename T, typename... Args>
    void create(Args... args) {
        set(std::make_shared<T>(args...));
    }

    template <typename T>
    void set(std::shared_ptr<T> v) {
        data = v;
        type = &typeid(T);  // The lifetime extends to the end of the program.
    }

    template <typename T>
    std::shared_ptr<T> getWriter() {
        if (*type != typeid(T)) {
            // Create by force
            create<T>();
        }
        return std::static_pointer_cast<T>(data);
    }

    template <typename T>
    std::shared_ptr<T> getReader() const {
        if (*type != typeid(T)) {
            // Invalid type cast
            throw(WrongTypeCast(typeid(T), *type));
        }
        return std::static_pointer_cast<T>(data);
    }

private:
    std::shared_ptr<void> data;
    const std::type_info *type = &typeid(void);
};

}  // namespace fase

#endif  // FASE_VARIABLE_H_20180617
