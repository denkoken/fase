
#ifndef FASE_GUI_H_20180622
#define FASE_GUI_H_20180622

#include "core.h"

namespace fase {

namespace pe {

class FaseGUI {
public:
    FaseGUI(FaseCore& fase);

    void start();

private:
    // TODO discuss whether to use raw pointer or std::weak_ptr.
    FaseCore* fase;
};

}  // namespace pe

}  // namespace fase

#endif  // GUI_H_20180622
