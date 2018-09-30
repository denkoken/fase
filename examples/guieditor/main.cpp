
#define FASE_USE_ADD_FUNCTION_BUILDER_MACRO
#include <fase.h>

#include <stdexcept>
#include <thread>

#include "fase_gl_utils.h"

FaseAutoAddingFunctionBuilder(Add, void Add(const int& a, const int& b,
                                            int& dst); );

void Add(const int& a, const int& b, int& dst) {
    dst = a + b; 
}

        struct X {
    int a = 6;
    size_t b = 123456789;
};

struct Y {
    Y(int a, float b, std::string c) : a(a), b(b), c(c) {}

    int a;
    float b;
    std::string c;
};

// clang-format off
FaseAutoAddingFunctionBuilder( Test0,
void Test0(int& test_dst,
           std::function<float(const int&, float&, float&)> f =
                   std::function<float(const int&, float&, float&)>(),
           const int& b = 42, const float c = 14.6f,
           const double d = 13.1,
           std::function<float(const int&, float&, float&)> f2 = {},
           int**** cdsc = nullptr, X x = {}, X x2 = {893, 987654321ul},
           Y y = Y(3, 10.f, "I'm full.")) {

    (void) test_dst; (void) f; (void) b; (void) c; (void) d; (void) f2;
    (void) cdsc;

    if (x.a != 6 || x.b != 123456789) {
        throw std::runtime_error(
                "auto FaseAutoAddingFunctionBuilder bug : empty brace");
    }
    if (x2.a != 893 || x2.b != 987654321) {
        throw std::runtime_error(
                "auto FaseAutoAddingFunctionBuilder bug : init brace");
    }
    if (y.a != 3 || y.b != 10.f || y.c != std::string("I'm full.")) {
        std::cout << y.a << std::endl;
        std::cout << y.b << std::endl;
        std::cout << y.c << std::endl;
        throw std::runtime_error(
                "auto FaseAutoAddingFunctionBuilder bug : constructor");
    }
}
);
// clang-format on

FaseAutoAddingFunctionBuilder(Square, void Square(const int& in, int& dst) {
    dst = in * in;
});

FaseAutoAddingFunctionBuilder(Print, void Print(const int& in) {
    std::cout << in << std::endl;
});

FaseAutoAddingFunctionBuilder(Wait, void Wait(const int& seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
});

FaseAutoAddingFunctionBuilder(Assert, void Assert(const int& a, const int& b) {
    if (a != b) {
        throw std::runtime_error("a is not equal to b!");
    }
});

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> app;

#if !defined(__cpp_if_constexpr) || !defined(__cpp_inline_variables)
    // Register functions
    FaseAddFunctionBuilder(app, Add, (const int&, const int&, int&),
                           ("in1", "in2", "out"));
    FaseAddFunctionBuilder(app, Square, (const int&, int&), ("in", "out"));
    FaseAddFunctionBuilder(app, Print, (const int&), ("in"));
    FaseAddFunctionBuilder(app, Wait, (const int&), ("seconds"));
    FaseAddFunctionBuilder(app, Assert, (const int&, const int&), ("a", "b"));
#endif

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
