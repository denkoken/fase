
#ifndef MANAGER_H_20190217
#define MANAGER_H_20190217

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include "variable.h"

namespace fase {

class PipelineAPI {
public:
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

    virtual bool supposeInput(std::size_t size) = 0;
    virtual bool supposeOutput(std::size_t size) = 0;

    virtual bool run() = 0;

    virtual const std::map<std::string, Node>& getNodes() const noexcept = 0;
};

struct FunctionUtils {
    std::vector<std::string> args_names;
    std::string              description;
};

class CoreManager {
public:
    CoreManager();

    CoreManager(const CoreManager&);
    CoreManager(CoreManager&);
    CoreManager(CoreManager&&);
    CoreManager& operator=(const CoreManager&);
    CoreManager& operator=(CoreManager&);
    CoreManager& operator=(CoreManager&&);

    ~CoreManager();

    bool addUnivFunc(const UnivFunc& func, const std::string& name,
                     std::vector<Variable>&&         default_args,
                     const std::vector<std::string>& arg_names,
                     const std::string&              description = {});

    PipelineAPI&       operator[](const std::string& name);
    const PipelineAPI& operator[](const std::string& name) const;

    std::vector<std::string>             getPipelineNames();
    std::map<std::string, FunctionUtils> getFunctionUtils();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace fase

#endif  // MANAGER_H_20190217
