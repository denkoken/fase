
#ifndef CALLABLE_IMPL_H_20181015
#define CALLABLE_IMPL_H_20181015

#include "callable.h"

namespace fase {

template <typename... Type, size_t... Seq>
void Callable::CallableReturn::get0(std::tuple<Type*...> dsts,
                                    std::index_sequence<Seq...>) const {
    dummy(*std::get<Seq>(dsts) = *data[Seq].template getReader<Type>()...);
}

template <typename... Type, size_t... Seq>
std::tuple<Type...> Callable::CallableReturn::get1(
        TypeSequence<Type...>, std::index_sequence<Seq...>) const {
    return {*data[Seq].template getReader<Type>()...};
}

template <typename... Dsts>
void Callable::CallableReturn::get(Dsts*... dsts) const {
    get0(std::tuple<Dsts*...>{dsts...},
         std::make_index_sequence<sizeof...(Dsts)>());
}

template <typename... Rets>
std::tuple<Rets...> Callable::CallableReturn::get() const {
    return get1(TypeSequence<Rets...>(),
                std::make_index_sequence<sizeof...(Rets)>());
}

template <typename... Args>
void Callable::fixInput(
        const std::array<std::string, sizeof...(Args)>& arg_names,
        const std::array<std::string, sizeof...(Args)>& reprs) {
    assert(arg_names.size() == sizeof...(Args));

    std::lock_guard<std::mutex> guard(core_mutex);

    getCore()->unlockInOut();

    size_t n_arg = getCore()->getNodes().at(InputNodeStr()).links.size();
    for (size_t i = 0; i < n_arg; i++) {
        getCore()->delInput(0);
    }

    std::vector<const std::type_info*> types = {&typeid(Args)...};
    std::vector<Variable> args = {Args()...};
    for (size_t i = 0; i < arg_names.size(); i++) {
        getCore()->addInput(arg_names[i], types[i], args[i], reprs[i]);
    }
    getCore()->lockInOut();
}

template <typename... Args>
void Callable::fixOutput(
        const std::array<std::string, sizeof...(Args)>& arg_names) {
    assert(arg_names.size() == sizeof...(Args));

    std::lock_guard<std::mutex> guard(core_mutex);

    getCore()->unlockInOut();

    size_t n_arg = getCore()->getNodes().at(OutputNodeStr()).links.size();
    for (size_t i = 0; i < n_arg; i++) {
        getCore()->delOutput(0);
    }

    std::vector<const std::type_info*> types = {&typeid(Args)...};
    std::vector<Variable> args = {Args()...};
    for (size_t i = 0; i < arg_names.size(); i++) {
        getCore()->addOutput(arg_names[i], types[i], args[i]);
    }
    getCore()->lockInOut();
}

template <typename... Args>
Callable::CallableReturn Callable::operator()(Args&&... args) {
    // lock mutex
    std::lock_guard<std::mutex> guard(core_mutex);

    return call(getCore()->getMainPipelineNameLastSelected(),
                std::forward<Args>(args)...);
}

inline auto Callable::operator[](const std::string& project) {
    return [this, project](auto&&... args) {
        // lock mutex
        std::lock_guard<std::mutex> guard(core_mutex);

        if (!exists(getCore()->getPipelineNames(), project)) {
            throw(std::runtime_error("Pipeline \"" + project +
                                     "\" is not exists!"));
        }

        return call(project, std::forward<decltype(args)>(args)...);
    };
}

template <typename... Args>
Callable::CallableReturn Callable::call(const std::string& pipeline,
                                        Args&&... args) {
    auto pcore = getCore();

    // buffer now project name.
    std::string project_buf = pcore->getCurrentPipelineName();

    // switch to call project, and run pipeline.
    pcore->switchPipeline(pipeline);

    pcore->setInput(std::forward<Args>(args)...);

    if (!pcore->build()) {
        pcore->switchPipeline(project_buf);
        throw(std::runtime_error("Pipeline building failed!"));
    }
    pcore->run();

    // switch to buffered project.
    pcore->switchPipeline(project_buf);

    return {pcore->getOutputs()};
}

}  // namespace fase

#endif  // CALLABLE_IMPL_H_20181015
