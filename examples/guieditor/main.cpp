
#define FASE_USE_ADD_FUNCTION_BUILDER_MACRO
#include <fase/callable.h>
#include <fase/editor.h>
#include <fase/fase.h>

#include <random>
#include <stdexcept>
#include <thread>
#include <valarray>
#include <vector>

#include "fase_gl_utils.h"

FaseAutoAddingFunctionBuilder(Add,
                              void Add(const int& a, const int& b, int& dst);)

        void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

struct X {
    int a = 6;
    size_t b = 123456789;
};

struct Y {
    Y(int a_, float b_, std::string c_) : a(a_), b(b_), c(c_) {}

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
           int**** cdsc = nullptr,
           X x = {}, // you can use initilizer,
           X x2 = {893, 987654321ul},
           Y y = Y(3, 10.f, "I'm full.") // and, constructor.
           ) {

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
)

FaseAutoAddingFunctionBuilder(Square, void Square(const int& in, int& dst) {
    dst = in * in;
})

FaseAutoAddingFunctionBuilder(Print, void Print(const int& in) {
    std::cout << in << std::endl;
})

FaseAutoAddingFunctionBuilder(Wait, void Wait(const int& seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
})

FaseAutoAddingFunctionBuilder(Assert, void Assert(const int& a, const int& b) {
    if (a != b) {
        throw std::runtime_error("a is not equal to b!");
    }
})

FaseAutoAddingFunctionBuilder(VARandom,
void VARandom(const size_t& size,
              std::valarray<double>& dst) {
    dst = std::valarray<double>(size);

    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_real_distribution<> rand100(0.0, 1.0);
    for (size_t i = 0; i < size; i++) {
        dst[i] = rand100(mt);
    }
})

FaseAutoAddingFunctionBuilder(VecRandom,
void VecRandom(const size_t& size,
               std::vector<double>& dst) {
    dst.resize(size);

    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_real_distribution<> rand100(0.0,
                                             1.0);
    for (size_t i = 0; i < size; i++) {
        dst[i] = rand100(mt);
    }
})

FaseAutoAddingFunctionBuilder(VASame,
void VASame(const std::valarray<double>& a, const std::valarray<double>& b,
            double& dst) {
    std::valarray<double> test(a.size());
    for (size_t j = 0; j < a.size(); j++) {
        test += (a - b);
    }
    dst = test.sum();
})

FaseAutoAddingFunctionBuilder(VAAdd,
void VAAdd(const std::valarray<double>& a,
           const std::valarray<double>& b,
           size_t n, std::valarray<double>& dst) {
    dst = std::valarray<double>(a.size());
    for (size_t j = 0; j < n; j++) {
        dst += a + b;
    }
})

FaseAutoAddingFunctionBuilder(VecAdd,
void VecAdd(const std::vector<double>& a,
            const std::vector<double>& b,
            size_t n,
            std::vector<double>& dst) {
    dst = std::vector<double>(a.size(), 0.0);
    for (size_t j = 0; j < n; j++) {
        for (size_t i = 0; i < a.size(); i++) {
            dst[i] += a[i] + b[i];
        }
    }
})

void LastPrintProjectRun(fase::Callable& app) {

    try {        // for catching nested error
        try {    // for catching ErrorThrownByNode

            // Both Type returning is OK!

            // Type 1.
            std::tuple<int, std::string> dst =
                    app(std::string("good morning!"), 3).get<int, std::string>();

            std::cout << "output1 : " << std::get<0>(dst) << std::endl
                      << "output2 : " << std::get<1>(dst) << std::endl;

            // Type 2.
            int dst1;
            std::string dst2;
            app(std::string("good bye!"), 7).get(&dst1, &dst2);

            std::cout << "output1 : " << dst1 << std::endl
                      << "output2 : " << dst2 << std::endl;

            // from c++17, you can use structure bindings, like down.
#ifdef __cpp_structured_bindings
            {
                // Type 1'.
                auto [dst1, dst2] =
                        app(std::string("hello!"), 17).get<int, std::string>();

                std::cout << "output1 : " << dst1 << std::endl
                          << "output2 : " << dst2 << std::endl;
            }
#endif
        } catch (fase::ErrorThrownByNode& e) {
            std::cerr << "Node " << e.node_name
                      << " throws Error;" << std::endl;
            e.rethrow_nested();
        }
    } catch (std::exception& e) {
        std::cerr << "    " << e.what() << std::endl;
    }
}

// clang-format on

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor, fase::Callable> app;

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
            [](const std::string& str) { return std::stoi(str); },
            [](const int& a) { return "int(" + std::to_string(a) + ")"; });

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    app.fixInput<std::string, int>({{"str", "num"}});
    app.fixOutput<int, std::string>({{"dst_num", "dst_str"}});

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, app);

    LastPrintProjectRun(app);

    return 0;
}
