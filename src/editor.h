
#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"
#include "variable.h"

namespace fase {

class Editor {
public:
    virtual void start(FaseCore*) = 0;
    virtual ~Editor() {}
};

namespace editor {

class CLIEditor : public Editor {
public:
    CLIEditor() : Editor() {}

    void start(FaseCore* core);
};

}  // namespace editor

}  // namespace fase

#endif  // EDITOR_H_20180628
