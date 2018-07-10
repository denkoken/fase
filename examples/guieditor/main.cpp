#include <fase.h>

#include "fase_gl_utils.h"
#include "fase_var_generators.h"

void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

void Print(const char& in) {
    std::cout << in << std::endl;
}
// void Print(const int& in) {
//     std::cout << in << std::endl;
// }

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> fase;

    // Register functions
    FaseAddFunctionBuilder(fase, Add, (const int&, const int&, int&),
                           ("in1", "in2", "out"));
    FaseAddFunctionBuilder(fase, Square, (const int&, int&), ("in", "out"));
    FaseAddFunctionBuilder(fase, Print, (const char&), ("in"));

    // Register for argument editing
    FaseInstallBasicGuiGenerators(fase);

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, [&]() {
        // Draw Fase's interface
        return fase.runEditing("Fase Editor", "##fase");
    });

    return 0;
}
