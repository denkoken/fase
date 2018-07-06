#ifndef EDITOR_GUI_H_20180628
#define EDITOR_GUI_H_20180628

#include "editor.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

namespace fase {

class GUIEditor : public EditorBase<GUIEditor> {
public:
    void start(FaseCore *core, GLFWwindow* window);

private:
};

}  // namespace fase

#endif  // EDITOR_GUI_H_20180628
