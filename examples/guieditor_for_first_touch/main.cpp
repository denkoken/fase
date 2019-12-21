
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>

#include "../fase_gl_utils.h"

namespace {

void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

void Print(const int& in) {
    std::cout << in << std::endl;
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
    FaseAddUnivFunction(Add, (const int&, const int&, int&),
                        ("in1", "in2", "out"), app);
    FaseAddUnivFunction(Square, (const int&, int&), ("in", "out"), app);
    FaseAddUnivFunction(Print, (const int&), ("in"), app);
    FaseAddUnivFunction(Assert, (const int&, const int&), ("a", "b"), app,
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
