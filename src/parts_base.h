
#ifndef PARTS_BASE_H_20181011
#define PARTS_BASE_H_20181011

#include <memory>
#include <mutex>

#include "core.h"
#include "type_utils.h"

namespace fase {

class PartsBase {
public:
    PartsBase(const TypeUtils& utils) : utils(utils) {}
    virtual ~PartsBase(){};

protected:
    /// this must be overridden by only Fase class.
    virtual std::shared_ptr<FaseCore> getCore() = 0;

    const TypeUtils& utils;

    std::mutex core_mutex;
};

}  // namespace fase

#endif  // PARTS_BASE_H_20181011
