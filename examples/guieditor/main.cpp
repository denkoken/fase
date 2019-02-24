
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>

#include <stdexcept>
#include <thread>
#include <valarray>
#include <vector>

#include "fase_gl_utils.h"

void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

void Print(const int& in) {
    std::cout << in << std::endl;
}

void Wait(const int& seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

void Assert(const int& a, const int& b) {
    if (a != b) {
        throw std::runtime_error("a is not equal to b!");
    }
}

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::ImGuiEditor> app;

#if 0
#if !defined(__cpp_if_constexpr) || !defined(__cpp_inline_variables)
    // Register functions
    FaseAddFunctionBuilder(app, Add, (const int&, const int&, int&),
                           ("in1", "in2", "out"));
    FaseAddFunctionBuilder(app, Square, (const int&, int&), ("in", "out"));
    FaseAddFunctionBuilder(app, Print, (const int&), ("in"));
    FaseAddFunctionBuilder(app, Wait, (const int&), ("seconds"));
    FaseAddFunctionBuilder(app, Assert, (const int&, const int&), ("a", "b"));
#endif
#endif

    auto bg_col = std::make_shared<std::vector<float>>(3);

    // add optional buttons.  [fase::GUIEditor]
    app.addOptinalButton("Print",
                         [] { std::cout << "hello world!" << std::endl; },
                         "say \"hello world!\" in command line");
    app.addOptinalButton("Reset BG",
                         [bg_col] {
                             (*bg_col)[0] = float(std::rand()) / RAND_MAX;
                             (*bg_col)[1] = float(std::rand()) / RAND_MAX;
                             (*bg_col)[2] = float(std::rand()) / RAND_MAX;
                         },
                         "Change a background color at random.\n"
                         "The mood will change too.");

    // add serializer/deserializer
    app.registerTextIO<int>(
            "int", [](const int& a) { return std::to_string(a); },
            [](const std::string& str) { return std::stoi(str); },
            [](const int& a) { return "int(" + std::to_string(a) + ")"; });

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, app, bg_col);

    return 0;
}
