
#ifndef CALLABLE_IMPL_H_20181015
#define CALLABLE_IMPL_H_20181015

#include "callable.h"

namespace fase {
template <typename... Dsts>
void Callable::CallableReturn::get(Dsts*... dsts) const {
    get0(std::tuple<Dsts*...>{dsts...},
         std::make_index_sequence<sizeof...(Dsts)>());
}

template <typename... Rets>
std::tuple<Rets...> Callable::CallableReturn::get() const {
    return get1(TypeSequence<std::tuple<Rets...>>(),
                std::make_index_sequence<sizeof...(Rets)>());
}

template <typename PointerTuple, size_t... Seq>
void Callable::CallableReturn::get0(PointerTuple dsts,
                                    std::index_sequence<Seq...>) const {
    dummy(*std::get<Seq>(dsts) =
                  *(data[Seq]
                            .template getReader<typename std::remove_reference<
                                    decltype(*std::get<Seq>(
                                            std::declval<PointerTuple>()))>::
                                                        type>())...);
}

template <class Tuple, size_t... Seq>
Tuple Callable::CallableReturn::get1(TypeSequence<Tuple>,
                                     std::index_sequence<Seq...>) const {
    return {*data[Seq]
                     .template getReader<typename std::remove_reference<
                             decltype(std::get<Seq>(
                                     std::declval<Tuple>()))>::type>()...};
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

    std::vector<Variable> output_a;  // output array
    call(&output_a, std::forward<Args>(args)...);

    return {std::move(output_a)};
}

inline auto Callable::operator[](const std::string& project) {
    return [this, project](auto&&... args) {
        // lock mutex
        std::lock_guard<std::mutex> guard(core_mutex);

        // buffer now project name.
        std::string project_buf = getCore()->getCurrentPipelineName();

        // switch to call project, and run pipeline.
        getCore()->switchPipeline(project);
        std::vector<Variable> output_a;
        this->call(&output_a, args...);

        // switch to buffered project.
        getCore()->switchPipeline(project_buf);
        return CallableReturn{std::move(output_a)};
    };
}

template <typename... Args>
void Callable::call(std::vector<Variable>* dst, Args&&... args) {
    auto pcore = getCore();

    pcore->setInput(std::forward<Args>(args)...);

    pcore->build();
    pcore->run();

    *dst = pcore->getOutputs();
}

}  // namespace fase

#endif  // CALLABLE_IMPL_H_20181015
