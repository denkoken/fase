#ifndef EXTENSIONS_H_20181014
#define EXTENSIONS_H_20181014

#include <memory>

#include "core_util.h"
#include "parts_base.h"

namespace fase {

class FaseCore;

/**
 * @defgroup FaseParts
 * @{
 */

/**
 * @brief
 *      You can touch FaseCore directly with this parts.
 *      But it is depricated.
 *      If you want to touch core,
 *      you should make and use the new Part extended PartsBase class.
 */
class DirectlyAccessibleBareCore : public PartsBase {
public:
    std::shared_ptr<FaseCore> getAccessibleCore() {
        return getCore();
    }
};

/**
 * @brief
 *      You can load the saved pipeline from *.txt file with this.
 */
class Loadable : public PartsBase {
public:
    /**
     * @brief
     *      You can load the saved pipeline from *.txt file with this.
     *
     * @param filename
     *      the file path.
     * @param target_name
     *      loaded pipeline' name in the core.
     *
     * @return
     *      succeeded or not.
     */
    bool load(const std::string& filename,
              const std::string& target_name = "") {
        std::lock_guard<std::mutex> lock(core_mutex);

        return LoadFaseCore(filename, getCore().get(), utils, target_name);
    }
};

/// @}

}  // namespace fase

#endif  // EXTENSIONS_H_20181014
