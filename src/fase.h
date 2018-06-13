#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>

namespace fase {

class Variable {
public:
    Variable() = default;

    template <typename T>
    Variable(T &&value)
        : data(std::make_shared<typename std::remove_reference<T>::type>(
              std::forward<T>(value))),
          type(&typeid(typename std::remove_reference<T>::type)) {}

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
            std::cerr << "Invalid cast (" << type->name() << " vs "
                      << typeid(T).name() << ")" << std::endl;
            return std::make_shared<T>();
        }
        return std::static_pointer_cast<T>(data);
    }

private:
    std::shared_ptr<void> data;
    const std::type_info *type = &typeid(void);
};

class FunctionNode {
public:
    virtual void build(const std::vector<Variable *> &args) = 0;
    virtual void apply() = 0;
};

template <typename... Args>
class StandardFunction : public FunctionNode {
public:
    template <class Callable>
    StandardFunction(Callable in_func) {
        func = std::function<void(Args...)>(in_func);
    }

    void build(const std::vector<Variable *> &in_args) {
        if (in_args.size() != sizeof...(Args)) {
            std::cerr << "Invalid arguments" << std::endl;
            return;
        }
        for (int i = 0; i < int(sizeof...(Args)); i++) {
            args[i] = *in_args[i];
        }
        binded = bind(func, std::make_index_sequence<sizeof...(Args)>());
    }

    void apply() { binded(); }

private:
#if 1
    template <typename Callable, typename Head, typename... Tail>
    auto bind(const Callable &func) {
        auto a = [func, &v = this->args[sizeof...(Args) - sizeof...(Tail) - 1]](
                     Tail... arguments) {
            func(*v.template getReader<
                     typename std::remove_reference<Head>::type>(),
                 std::forward<Tail>(arguments)...);
        };
        if constexpr (sizeof...(Tail) == 0) {
            return a;
        } else {
            return bind<typename std::remove_reference<decltype(a)>::type,
                        Tail...>(a);
        }
    }
#else  // more readable, with same process.
    template <class Callable, class Head, class... Tail>
    struct Wrapper {
        Callable func;
        Variable &arg1;
        Wrapper(const Callable &func_, Variable &arg1_)
            : func(func_), arg1(arg1_) {}

        void operator()(Tail... arguments) {
            using arg1_type = typename std::remove_reference<Head>::type;
            func(*arg1.template getReader<arg1_type>(),
                 std::forward<Tail>(arguments)...);
        }
    };
    template <typename Callable, typename Head, typename... Tail>
    auto bind(const Callable &func) {
        Wrapper<Callable, Head, Tail...> a(
            func, this->args[sizeof...(Args) - sizeof...(Tail) - 1]);
        if constexpr (sizeof...(Tail) == 0) {
            return a;
        } else {
            return bind<Wrapper<Callable, Head, Tail...>, Tail...>(a);
        }
    }
#endif

    std::function<void(Args...)> func;
    std::function<void()> binded;
    std::array<Variable, sizeof...(Args)> args;
};

}  // namespace fase
