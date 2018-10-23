#ifndef EXTENSIONS_H_20181014
#define EXTENSIONS_H_20181014

#include <memory>

#include "parts_base.h"

namespace fase {

class FaseCore;

/**
 * @defgroup FaseParts
 * @{
 */
class DirectlyAccessibleBareCore : public PartsBase {
public:
    std::shared_ptr<FaseCore> getAccessibleCore() {
        return getCore();
    }
};
/// @}

}  // namespace fase

#endif  // EXTENSIONS_H_20181014
