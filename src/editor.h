
#ifndef EDITOR_H_20180628
#define EDITOR_H_20180628

#include "core.h"
#include "variable.h"

namespace fase {

namespace develop {

class Editor {
public:
    virtual void start(pe::FaseCore*, Variable) = 0;
};

}  // namespace develop

namespace editor {

class CLIEditor : public develop::Editor {
public:
    CLIEditor() : Editor() {}

    void start(pe::FaseCore* core, Variable variable);
};

}  // namespace editor

}  // namespace fase

#endif  // EDITOR_H_20180628
