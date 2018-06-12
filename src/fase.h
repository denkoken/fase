#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>

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
        binded = bind(func, args);
    }
    void apply() { binded(); }

private:
    template <std::size_t N>
    auto bind(std::function<void(Args...)>& f, std::array<Variable, N>& args) {
        return bind(f, args, std::make_index_sequence<N>());
    }

    template<std::size_t N, std::size_t... Idx>
    auto bind(std::function<void(Args...)>& f, std::array<Variable, N>& args,
               std::index_sequence<Idx...>) {
        return std::bind(f, *args[Idx].template getReader<Args>()...);
    }

    std::function<void(Args...)> func;
    std::function<void()> binded;
    std::array<Variable, sizeof...(Args)> args;
};
