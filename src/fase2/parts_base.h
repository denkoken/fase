
#ifndef PARTS_BASE_H_20190223
#define PARTS_BASE_H_20190223

#include <shared_mutex>

#include "manager.h"

namespace fase {

class PartsBase {
protected:
    virtual std::tuple<std::shared_lock<std::shared_mutex>,
                       std::shared_ptr<const CoreManager>>
    getReader() = 0;

    virtual std::tuple<std::unique_lock<std::shared_mutex>,
                       std::shared_ptr<CoreManager>>
    getWriter() = 0;

    virtual bool init() {
        return true;
    }
};

}  // namespace fase

#endif  // PARTS_BASE_H_20190223
