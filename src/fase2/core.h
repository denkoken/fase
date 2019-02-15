
#ifndef CORE_H_20190205
#define CORE_H_20190205

#include <exception>
#include <functional>
#include <string>
#include <vector>

#include "variable.h"

namespace fase {

using UnivFunc = std::function<void(std::vector<Variable>&)>;

template <typename... Args>
class UnivFuncGenerator {
public:
    template <typename Callable>
    static auto Gen(Callable&& f) {
        return Wrap(std::forward<Callable>(f),
                    std::index_sequence_for<Args...>());
    }

private:
    template <std::size_t... Seq, typename Callable>
    static auto Wrap(Callable&& f, std::index_sequence<Seq...>) {
        auto fp = std::make_shared<std::decay_t<Callable>>(
                std::forward<std::decay_t<Callable>>(f));
        return [fp](std::vector<Variable>& args) {
            if (args.size() != sizeof...(Args)) {
                throw std::logic_error(
                        "Invalid size of variables at UnivFunc.");
            }
            (*fp)(static_cast<Args>(
                    *args[Seq].getWriter<std::decay_t<Args>>())...);
        };
    }
};

class Core {
public:
    Core();
    ~Core();

    // ======= unstable API =========
    bool addUnivFunc(UnivFunc&& worker, const std::string& name,
                     std::vector<Variable>&& default_args);

    // ======= stable API =========
    bool newNode(const std::string& name) noexcept;
    bool delNode(const std::string& name) noexcept;

    bool setArgument(const std::string& node, std::size_t idx, Variable& var);
    bool setPriority(const std::string& node, int priority);

    bool allocateWork(const std::string& work, const std::string& node);
    bool linkNode(const std::string& src_node, std::size_t src_arg,
                  const std::string& dst_node, std::size_t dst_arg);
    bool unlinkNode(const std::string& dst_node, std::size_t dst_arg);

    bool supposeInput(std::vector<Variable>& vars);
    bool supposeOutput(std::vector<Variable>& vars);

    bool run();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace fase

#endif  // CORE_H_20190205
