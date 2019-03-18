
#ifndef MANAGER_H_20190217
#define MANAGER_H_20190217

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include "core.h"
#include "utils.h"
#include "variable.h"

namespace fase {

class CoreManager;

class ExportedPipe {
public:
    ExportedPipe(Core&& core_, std::function<void(Core*)>&& reseter_)
        : core(std::move(core_)), reseter(std::move(reseter_)) {}

    void operator()(std::vector<Variable>& vs);
    void reset() {
        reseter(&core);
    }

private:
    Core                       core;
    std::function<void(Core*)> reseter;
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
                     std::vector<Variable>&& default_args,
                     FunctionUtils&&         utils);

    PipelineAPI&       operator[](const std::string& name);
    const PipelineAPI& operator[](const std::string& name) const;

    void        setFocusedPipeline(const std::string& pipeline_name);
    std::string getFocusedPipeline() const;

    ExportedPipe exportPipe(const std::string& name) const;

    std::vector<std::string>             getPipelineNames() const;
    std::map<std::string, FunctionUtils> getFunctionUtils(
            const std::string& p_name) const;
    const DependenceTree& getDependingTree() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace fase

#endif  // MANAGER_H_20190217
