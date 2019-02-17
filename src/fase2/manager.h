
#ifndef MANAGER_H_20190217
#define MANAGER_H_20190217

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include "variable.h"

namespace fase {

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
