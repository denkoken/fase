
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>

#include "../fase_gl_utils.h"

namespace {

int Add(const int& a, const int& b) {
    return a + b;
}

int Square(const int& in) {
    return in * in;
}

bool Print(const int& in) {
    std::cout << in << std::endl;
    return true;
}

void Assert(const int& a, const int& b) {
    if (a != b) {
        throw std::runtime_error("a is not equal to b!");
    }
}

} // namespace

int main() {
    // clang-format on
    // Create Fase instance with GUI editor
    fase::Fase<fase::ImGuiEditor> app;

    // Register functions
    FaseAddUnivFunction(Add, int(const int&, const int&), ("in1", "in2", "out"),
                        app);
    FaseAddUnivFunction(Square, int(const int&), ("in", "out"), app);
    FaseAddUnivFunction(Print, bool(const int&), ("in"), app);
    FaseAddUnivFunction(Assert, void(const int&, const int&), ("a", "b"), app,
                        "assert(a == b)", {1, 1});

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, app);

    return 0;
}
