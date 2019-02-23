
#ifndef COMMON_H_20190217
#define COMMON_H_20190217

#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "variable.h"

namespace fase {

struct Report {
    using TimeType = decltype(std::chrono::system_clock::now() -
                              std::chrono::system_clock::now());
    TimeType execution_time;  // sec

    std::map<std::string, Report> child_reports;
};

using UnivFunc = std::function<void(std::vector<Variable>&, Report*)>;

struct Node {
    std::string           func_name = "";
    UnivFunc              func = [](auto&, auto) {};
    std::vector<Variable> args;
    int                   priority = 0;
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

    virtual bool run(Report* preport = nullptr) = 0;

    virtual const std::map<std::string, Node>& getNodes() const noexcept = 0;
    virtual const std::vector<Link>&           getLinks() const noexcept = 0;
};

struct TypeStringConverters {
    using Serializer = std::function<std::string(const Variable&)>;
    using Deserializer = std::function<void(Variable&, const std::string&)>;
    using Checker = std::function<bool(const Variable&)>;
    using DefMaker = Serializer;

    Serializer   serializer;
    Deserializer deserializer;
    Checker      checker;
    DefMaker     def_maker;

    std::string name;
};

}  // namespace fase

#endif  // COMMON_H_20190217
