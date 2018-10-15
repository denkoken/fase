#ifndef CALLABLE_H_20181014
#define CALLABLE_H_20181014

#include <tuple>

#include "core_util.h"
#include "parts_base.h"
#include "variable.h"

namespace fase {

class Callable : public PartsBase {
    class CallableReturn {
    public:
        template <typename... Dsts>
        void get(Dsts*... dsts) const;

        template <typename... Rets>
        std::tuple<Rets...> get() const;

        const std::vector<Variable> data;

    private:
        template <typename... Types>
        struct TypeSequence {};
        template <typename... Types>
        void dummy(Types...) const {}

        template <typename PointerTuple, size_t... Seq>
        void get0(PointerTuple dsts, std::index_sequence<Seq...>) const;
        template <class Tuple, size_t... Seq>
        Tuple get1(TypeSequence<Tuple>, std::index_sequence<Seq...>) const;
    };

public:
    Callable(const TypeUtils& utils_) : PartsBase(utils_) {}

    template <typename... Args>
    void fixInput(const std::string& project,
                  const std::array<std::string, sizeof...(Args)>& arg_names,
                  const std::array<std::string, sizeof...(Args)>& reprs = {});

    template <typename... Args>
    void fixOutput(const std::string& project,
                   const std::array<std::string, sizeof...(Args)>& arg_names);

    template <typename... Args>
    CallableReturn operator()(Args&&... args);

    /**
     * @brief
     *      calling project choice with this.
     *      use like down.
     *      ```
     *      FaseCore<Callable, [...]> app;
     *      ...
     *      app["the project I want to call"](...);
     *      ```
     *
     * @param project
     *      project name string, you want to call.
     *
     * @return
     *      lambda instance. the returned can be called like above operator().
     *
     * Registering returned object is dengerous, if you will delete projects.
     */
    auto operator[](const std::string& project);

private:
    template <typename... Args>
    void call(std::vector<Variable>* dst, Args&&... args);
};

}  // namespace fase

#include "callable_impl.h"

#endif  // CALLABLE_H_20181014
