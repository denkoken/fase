
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
        CallableReturn(const CallableReturn&) = delete;
        CallableReturn(CallableReturn&) = delete;
        CallableReturn(CallableReturn&&) = delete;
        CallableReturn& operator=(const CallableReturn&) = delete;
        CallableReturn& operator=(CallableReturn&) = delete;
        CallableReturn& operator=(CallableReturn&&) = delete;

        template <typename... Dsts>
        void get(Dsts*... dsts) const {
            get0(std::tuple<Dsts*...>{dsts...},
                 std::make_index_sequence<sizeof...(Dsts)>());
        }

        template <typename... Rets>
        std::tuple<Rets...> get() const {
            return get1(TypeSequence<std::tuple<Rets...>>(),
                        std::make_index_sequence<sizeof...(Rets)>());
        }

        const std::vector<Variable> data;

    private:
        template <typename... Types>
        struct TypeSequence {};
        template <typename... Types>
        void dummy(Types...) const {}

        template <typename PointerTuple, size_t... Seq>
        void get0(PointerTuple dsts, std::index_sequence<Seq...>) const {
            dummy(*std::get<Seq>(dsts) = *(
                          data[Seq]
                                  .template getReader<
                                          typename std::remove_reference<decltype(
                                                  *std::get<
                                                          Seq>(std::declval<
                                                               PointerTuple>()))>::
                                                  type>())...);
        }

        template <class Tuple, size_t... Seq>
        Tuple get1(TypeSequence<Tuple>, std::index_sequence<Seq...>) const {
            return {*data[Seq]
                             .template getReader<typename std::remove_reference<
                                     decltype(std::get<Seq>(
                                             std::declval<Tuple>()))>::
                                                         type>()...};
        }
    };

public:
    Callable(const TypeUtils& utils) : PartsBase(utils) {}

    template <typename... Args>
    void fixInput(const std::string& project,
                  const std::array<std::string, sizeof...(Args)>& arg_names,
                  const std::array<std::string, sizeof...(Args)>& reprs = {}) {
        assert(arg_names.size() == sizeof...(Args));
        std::lock_guard<std::mutex> guard(core_mutex);
        auto project_buf = getCore()->getProjectName();
        getCore()->switchProject(project);
        getCore()->unlockInOut();

        size_t n_arg = getCore()->getNodes().at(InputNodeStr()).links.size();
        for (size_t i = 0; i < n_arg; i++) {
            getCore()->delInput(0);
        }

        for (const auto& arg_name : arg_names) {
            getCore()->addInput(arg_name);
        }

        std::vector<Variable> args = {Args()...};
        for (size_t i = 0; i < sizeof...(Args); i++) {
            getCore()->setNodeArg(InputNodeStr(), i, reprs[i], args[i]);
        }
        getCore()->lockInOut();
        getCore()->switchProject(project_buf);
    }

    template <typename... Args>
    void fixOutput(const std::string& project,
                   const std::array<std::string, sizeof...(Args)>& arg_names) {
        std::lock_guard<std::mutex> guard(core_mutex);
        auto project_buf = getCore()->getProjectName();
        getCore()->switchProject(project);
        getCore()->unlockInOut();

        size_t n_arg = getCore()->getNodes().at(OutputNodeStr()).links.size();
        for (size_t i = 0; i < n_arg; i++) {
            getCore()->delOutput(0);
        }

        for (const auto& arg_name : arg_names) {
            getCore()->addOutput(arg_name);
        }
        std::vector<Variable> args = {Args()...};
        for (size_t i = 0; i < sizeof...(Args); i++) {
            getCore()->setNodeArg(OutputNodeStr(), i, "", args[i]);
        }
        getCore()->lockInOut();
        getCore()->switchProject(project_buf);
    }

    template <typename... Args>
    CallableReturn operator()(Args&&... args) {
        // lock mutex
        std::lock_guard<std::mutex> guard(core_mutex);

        std::vector<Variable> output_a;  // output array
        call(&output_a, std::forward<Args>(args)...);

        return {std::move(output_a)};
    }

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
    auto operator[](const std::string& project) {
        return [this, project](auto&&... args) {
            // lock mutex
            std::lock_guard<std::mutex> guard(core_mutex);

            // buffer now project name.
            std::string project_buf = getCore()->getProjectName();

            // switch to call project, and run pipeline.
            getCore()->switchProject(project);
            std::vector<Variable> output_a;
            this->call(&output_a, args...);

            // switch to buffered project.
            getCore()->switchProject(project_buf);
            return CallableReturn{std::move(output_a)};
        };
    }

private:
    template <typename... Args>
    void call(std::vector<Variable>* dst, Args&&... args) {
        std::array<Variable, sizeof...(Args)> arg_vars = {
                {Variable(std::forward<Args>(args))...}};

        auto pcore = getCore();

        const size_t n_input =
                pcore->getNodes().at(InputNodeStr()).links.size();
        for (size_t i = 0; i < n_input; i++) {
            pcore->setNodeArg(InputNodeStr(), i, "", arg_vars[i]);
        }

        pcore->build();
        pcore->run();

        *dst = pcore->getOutputs();
    }
};

}  // namespace fase

#endif  // CALLABLE_H_20181014
