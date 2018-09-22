#include <fase.h>

#include <thread>

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

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> app;

    // Register functions
    FaseAddFunctionBuilder(app, Add, (const int&, const int&, int&),
                           ("in1", "in2", "out"));
    FaseAddFunctionBuilder(app, Square, (const int&, int&), ("in", "out"));
    FaseAddFunctionBuilder(app, Print, (const int&), ("in"));
    FaseAddFunctionBuilder(app, Wait, (const int&), ("seconds"));

    app.registerTextIO<int>(
            "int", [](const int& a) { return std::to_string(a); },
            [](const std::string& str) { return std::stoi(str); });

    // app.registerConstructorAndVieweditor<int>("int",
    //                     [](const int& a) { return std::to_string(a); },
    //                     [](const char*, const int&) -> std::unique_ptr<int> {
    //                     return {}; });

    app.setupEditor();

    // Register for argument editing
    // FaseInstallBasicGuiGenerators(app);

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
        return app.runEditing("Fase Editor", "##fase");
    });

    return 0;
}
