
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>
#include <fase2/stdparts.h>

#include <random>
#include <stdexcept>
#include <thread>
#include <valarray>
#include <vector>

#ifdef USE_NFD
#include "extra_parts.h"
#endif

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

class Z {
private:
    Z() {}

public:
    static Z createZ() {
        return Z();
    }
};

// clang-format off
FaseAutoAddingUnivFunction( Test0,
void Test0(int& test_dst,
           Z& z,
           std::function<float(const int&, float&, float&)> f =
                   std::function<float(const int&, float&, float&)>(),
           const int& b = 42,
           const float c = 14.6f,
           const double d = 13.1,
           std::function<void(const double&, int*)> f2 = {},
           int**** cdsc = nullptr,
           X x = {}, // you can use initilizer,
           X x2 = {893, 987654321ul},
           Y y = Y(3, 10.f, "I'm full.") // and, constructor.
           ) {

    (void) test_dst; (void) f; (void) b; (void) c; (void) d; (void) f2;
    (void) cdsc; (void) z;

    if (x.a != 6 || x.b != 123456789) {
        throw std::runtime_error(
                "auto FaseAutoAddingUnivFunction bug : empty brace");
    }
    if (x2.a != 893 || x2.b != 987654321) {
        throw std::runtime_error(
                "auto FaseAutoAddingUnivFunction bug : init brace");
    }
    if (y.a != 3 || y.b != 10.f || y.c != std::string("I'm full.")) {
        std::cout << y.a << std::endl;
        std::cout << y.b << std::endl;
        std::cout << y.c << std::endl;
        throw std::runtime_error(
                "auto FaseAutoAddingUnivFunction bug : constructor");
    }
}
)

FaseAutoAddingUnivFunction(VARandom,
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

FaseAutoAddingUnivFunction(VecRandom,
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

FaseAutoAddingUnivFunction(VASame,
void VASame(const std::valarray<double>& a, const std::valarray<double>& b,
            double& dst) {
    std::valarray<double> test(a.size());
    for (size_t j = 0; j < a.size(); j++) {
        test += (a - b);
    }
    dst = test.sum();
})

FaseAutoAddingUnivFunction(VAAdd,
void VAAdd(const std::valarray<double>& a,
           const std::valarray<double>& b,
           size_t n, std::valarray<double>& dst) {
    if (a.size() != b.size()) {
        return;
    }
    dst = std::valarray<double>(a.size());
    for (size_t j = 0; j < n; j++) {
        dst += a + b;
    }
})

FaseAutoAddingUnivFunction(VecAdd,
void VecAdd(const std::vector<double>& a,
            const std::vector<double>& b,
            size_t n,
            std::vector<double>& dst) {
    if (a.size() != b.size()) {
        return;
    }
    dst = std::vector<double>(a.size(), 0.0);
    for (size_t j = 0; j < n; j++) {
        for (size_t i = 0; i < a.size(); i++) {
            dst[i] += a[i] + b[i];
        }
    }
})


int main() {
    // clang-format on
    // Create Fase instance with GUI editor
    fase::Fase<fase::ImGuiEditor, fase::HardCallableParts<int>,
               fase::ExportableParts
#ifdef USE_NFD
               ,
               NFDParts
#endif
               >
            app;

    // Register functions
    FaseAddUnivFunction(Add, (const int&, const int&, int&),
                        ("in1", "in2", "out"), app);
    FaseAddUnivFunction(Square, (const int&, int&), ("in", "out"), app);
    FaseAddUnivFunction(Print, (const int&), ("in"), app);
    FaseAddUnivFunction(Wait, (const int&), ("seconds"), app,
                        "wait for \"seconds\".");
    FaseAddUnivFunction(Assert, (const int&, const int&), ("a", "b"), app,
                        "assert(a == b)", {1, 1});
    struct A {
        int count = 0;
        void operator()(int& dst) {
            dst = count++;
        }
    } counter;
    FaseAddUnivFunction(counter, (int&), ("count"), app);

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

    app.addOptinalButton(
            "Export pipe test",
            [&] {
                try {
                    auto exported = app.exportPipe();
                    std::vector<fase::Variable> vs(3, std::make_unique<int>());
                    *vs[0].getWriter<int>() = 2;
                    *vs[1].getWriter<int>() = 6;
                    if (!exported(vs)) {
                        throw std::runtime_error("invalid input/output form!");
                    }
                    std::cout << "1st result : " << *vs[2].getReader<int>()
                              << std::endl;
                    exported(vs);
                    std::cout << "2nd result : " << *vs[2].getReader<int>()
                              << std::endl;
                    exported(vs);
                    std::cout << "3rd result : " << *vs[2].getReader<int>()
                              << std::endl;
                    exported(vs);
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
            },
            "Export fucused pipeline as int(int, int), \n"
            "and call exported(2, 6) three times.");

    app.addOptinalButton(
            "Export pipe test (Hard Wraped)",
            [&] {
                try {
                    auto exported = fase::ToHard<int>::Pipe<int, int>::Gen(
                            app.exportPipe());
                    auto [res1] = exported(2, 6);
                    std::cout << "1st result : " << res1 << std::endl;
                    auto [res2] = exported(2, 6);
                    std::cout << "2nd result : " << res2 << std::endl;
                    auto [res3] = exported(2, 6);
                    std::cout << "3rd result : " << res3 << std::endl;
                    exported.reset();
                    auto [res4] = exported(2, 6);
                    std::cout << "4th result : " << res4 << std::endl;
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
            },
            "Export fucused pipeline as int(int, int), \n"
            "and call exported(2, 6) three times,\n"
            "and reset, and call one more");

#ifdef USE_NFD
    app.addOptinalButton("Load Pipeline", [&] { app.loadPipelineWithNFD(); },
                         "Load a pipeline with NativeFileDialog.");
    app.addOptinalButton("Save Pipeline", [&] { app.savePipelineWithNFD(); },
                         "Save a focused pipeline with NativeFileDialog.");
#endif

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

    try {
        auto [dst] = app.callHard(3, 4);
        std::cout << "app.callHard(3, 4) : " << dst << std::endl;
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        std::cout << " : It almost print \"HardCallableParts : "
                     "input/output type "
                     "isn't match!\"."
                  << std::endl;
    }

    return 0;
}
