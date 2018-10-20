#ifndef CALLABLE_H_20181014
#define CALLABLE_H_20181014

#include <tuple>

#include "core_util.h"
#include "parts_base.h"
#include "variable.h"

namespace fase {

/**
 * @defgroup FaseParts
 * @{
 */
class Callable : public PartsBase {
    class CallableReturn {
    public:
        /**
         * @brief
         *      get returned.
         *
         * @param dsts
         *      the pointers assigned returned by pipeline.
         */
        template <typename... Dsts>
        void get(Dsts*... dsts) const;

        /**
         * @brief
         *      get returned.
         *
         * @tparam Rets
         *      types of returned.
         */
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

    /**
     * @brief
     *      fix input of pipeline.
     *
     * @tparam Args
     *      types of fixed arguments.
     * @param project
     *      the project be fixed inputs.
     * @param arg_names
     *      input names.
     * @param reprs
     *      input representations.
     */
    template <typename... Args>
    void fixInput(const std::string& project,
                  const std::array<std::string, sizeof...(Args)>& arg_names,
                  const std::array<std::string, sizeof...(Args)>& reprs = {});

    /**
     * @brief
     *      fix output of pipeline.
     *
     * @tparam Args
     *      types of fixed arguments.
     * @param project
     *      the project be fixed outputs.
     * @param arg_names
     *      output names.
     */
    template <typename... Args>
    void fixOutput(const std::string& project,
                   const std::array<std::string, sizeof...(Args)>& arg_names);

    /**
     * @brief
     *      call current project pipeline.
     *
     * @tparam Args
     *      input argument types.
     * @param args
     *      input arguments.
     *
     * @return
     *      output of pipelines. look CallableReturn class to know how to use.
     */
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

    /**
     * @brief
     *      set current project.
     */
    void setProject(const std::string& project_name) {
        std::lock_guard<std::mutex> guard(core_mutex);
        getCore()->switchPipeline(project_name);
    }

private:
    template <typename... Args>
    void call(std::vector<Variable>* dst, Args&&... args);
};
/// @}

}  // namespace fase

#include "callable_impl.h"

#endif  // CALLABLE_H_20181014