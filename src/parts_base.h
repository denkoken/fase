
#ifndef PARTS_BASE_H_20181011
#define PARTS_BASE_H_20181011

#include <mutex>

#include "core.h"

namespace fase {

class PartsBase {
public:
    PartsBase() {}
    virtual ~PartsBase() {};

protected:
    /// this must be overridden by only Fase class.
    virtual FaseCore* getCore() = 0;

    std::mutex core_mutex;
};

}

#endif // PARTS_BASE_H_20181011
