
#ifndef CORE_H_20190205
#define CORE_H_20190205

#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "common.h"
#include "variable.h"

namespace fase {

class Core {
public:
    Core();
    ~Core();

    // ======= unstable API =========
    bool addUnivFunc(const UnivFunc& func, const std::string& name,
                     std::vector<Variable>&& default_args);

    // ======= stable API =========
    bool newNode(const std::string& name);
    bool renameNode(const std::string& old_name, const std::string& new_name);
    bool delNode(const std::string& name);

    bool setArgument(const std::string& node, std::size_t idx, Variable& var);
    bool setPriority(const std::string& node, int priority);

    bool allocateFunc(const std::string& work, const std::string& node);
    bool linkNode(const std::string& src_node, std::size_t src_arg,
                  const std::string& dst_node, std::size_t dst_arg);
    bool unlinkNode(const std::string& dst_node, std::size_t dst_arg);

    bool supposeInput(std::vector<Variable>& vars);
    bool supposeOutput(std::vector<Variable>& vars);

    bool run(Report* preport = nullptr);

    const std::map<std::string, Node>& getNodes() const noexcept;
    const std::vector<Link>&           getLinks() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}  // namespace fase

#endif  // CORE_H_20190205
