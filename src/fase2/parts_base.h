
#ifndef PARTS_BASE_H_20190223
#define PARTS_BASE_H_20190223

#include <memory>
#include <shared_mutex>

#include "manager.h"

namespace fase {

class PartsBase {
protected:
    PartsBase() = default;
    virtual ~PartsBase() = default;

    virtual std::tuple<std::shared_lock<std::shared_timed_mutex>,
                       std::shared_ptr<const CoreManager>>
    getReader(const std::chrono::nanoseconds& wait_time =
                      std::chrono::nanoseconds{-1}) const = 0;
    virtual std::tuple<std::unique_lock<std::shared_timed_mutex>,
                       std::shared_ptr<CoreManager>>
    getWriter(const std::chrono::nanoseconds& wait_time =
                      std::chrono::nanoseconds{-1}) = 0;

    virtual const TSCMap& getConverterMap() = 0;

    virtual bool init() {
        return true;
    }
};

} // namespace fase

#endif // PARTS_BASE_H_20190223
