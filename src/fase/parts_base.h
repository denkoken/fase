#ifndef PARTS_BASE_H_20181011
#define PARTS_BASE_H_20181011

#include <memory>
#include <mutex>

#include "type_utils.h"

namespace fase {

class FaseCore;

class PartsBase {
public:
    PartsBase(const TypeUtils& utils_) : utils(utils_) {}
    virtual ~PartsBase() {}

protected:
    /// this must be overridden by only Fase class.
    virtual std::shared_ptr<FaseCore> getCore() = 0;

    const TypeUtils& utils;

    std::mutex core_mutex;
};

}  // namespace fase

#endif  // PARTS_BASE_H_20181011
