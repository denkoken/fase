
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
    TimeType execution_time;
    float    getSec() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                       execution_time)
                       .count() *
               1e-6f;
    }

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

// Function Object Generator Type
enum struct FOGtype {
    Pure,
    Lambda,
    IndependingClass,
    DependingClass,
    OtherPipe,
    Special,
};

struct FunctionUtils {
    std::vector<std::string>         arg_names;
    std::vector<std::type_index>     arg_types;
    std::vector<bool>                is_input_args;
    FOGtype                          type;
    std::string                      repr;
    std::shared_ptr<std::type_index> callable_type;
    std::string                      arg_types_repr;
    std::string                      description;
};

class Core;

class CoreManager;

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

    virtual bool call(std::vector<Variable>& args) = 0;

    virtual bool run(Report* preport = nullptr) = 0;

    virtual const std::map<std::string, Node>&   getNodes() const noexcept = 0;
    virtual const std::vector<Link>&             getLinks() const noexcept = 0;
    virtual std::map<std::string, FunctionUtils> getFunctionUtils() const = 0;
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

using TSCMap = std::map<std::type_index, TypeStringConverters>;

std::vector<std::vector<std::string>>
GetRunOrder(const std::map<std::string, Node>& nodes,
            const std::vector<Link>&           links);

std::string GenNativeCode(const std::string& pipeline_name,
                          const CoreManager& cm, const TSCMap& utils,
                          const std::string& entry_name = "",
                          const std::string& indent = "    ");

std::string PipelineToString(const std::string& pipeline_name,
                             const CoreManager& cm, const TSCMap& utils);

bool LoadPipelineFromString(const std::string& str, CoreManager* pcm,
                            const TSCMap& utils);

bool SavePipeline(const std::string& pipeline_name, const CoreManager& cm,
                  const std::string& filename, const TSCMap& utils);

bool LoadPipelineFromFile(const std::string& filename, CoreManager* pcm,
                          const TSCMap& utils);

} // namespace fase

#endif // COMMON_H_20190217
