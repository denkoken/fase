#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"

namespace fase {

template <typename Parent>
class EditorBase {
public:
    template <typename Gen>
    bool addVarGenerator(Gen &&) {}

    template <typename... Args>
    void start(FaseCore *, Args...) {}
};

}  // namespace fase

#endif  // EDITOR_H_20180628
