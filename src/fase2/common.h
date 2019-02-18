
#ifndef COMMON_H_20190217
#define COMMON_H_20190217

#include <functional>
#include <map>
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

struct Node {
    std::string           func_name;
    std::vector<Variable> args;
    int                   priority;
};

struct Link {
    std::string src_node;
    std::size_t src_arg;
    std::string dst_node;
    std::size_t dst_arg;
};

class PipelineAPI {
public:
    virtual ~PipelineAPI() {}

    virtual bool newNode(const std::string& name) = 0;
    virtual bool renameNode(const std::string& old_name,
                            const std::string& new_name) = 0;
    virtual bool delNode(const std::string& name) = 0;

    virtual bool setArgument(const std::string& node, std::size_t idx,
                             Variable& var) = 0;
    virtual bool setPriority(const std::string& node, int priority) = 0;

    virtual bool allocateFunc(const std::string& work,
                              const std::string& node) = 0;

    virtual bool smartLink(const std::string& src_node, std::size_t src_arg,
                           const std::string& dst_node,
                           std::size_t        dst_arg) = 0;
    virtual bool unlinkNode(const std::string& dst_node,
                            std::size_t        dst_arg) = 0;

    virtual bool supposeInput(const std::vector<std::string>& arg_names) = 0;
    virtual bool supposeOutput(const std::vector<std::string>& arg_names) = 0;

    virtual bool run() = 0;

    virtual const std::map<std::string, Node>& getNodes() const noexcept = 0;
    virtual const std::vector<Link>&           getLinks() const noexcept = 0;
};

class FaildDummy : public PipelineAPI {
    bool newNode(const std::string&) override {
        return false;
    }
    bool renameNode(const std::string&, const std::string&) override {
        return false;
    }
    bool delNode(const std::string&) override {
        return false;
    }

    bool setArgument(const std::string&, size_t, Variable&) override {
        return false;
    }
    bool setPriority(const std::string&, int) override {
        return false;
    }

    bool allocateFunc(const std::string&, const std::string&) override {
        return false;
    }

    bool smartLink(const std::string&, size_t, const std::string&,
                   size_t) override {
        return false;
    }
    bool unlinkNode(const std::string&, size_t) override {
        return false;
    }

    bool supposeInput(const std::vector<std::string>&) override {
        return false;
    }
    bool supposeOutput(const std::vector<std::string>&) override {
        return false;
    }

    bool run() override {
        return false;
    }

    const std::map<std::string, Node>& getNodes() const noexcept override {
        return dum_n;
    }

    const std::vector<Link>& getLinks() const noexcept override {
        return dum_l;
    }

private:
    std::map<std::string, Node> dum_n;
    std::vector<Link>           dum_l;
};

}  // namespace fase

#endif  // COMMON_H_20190217
