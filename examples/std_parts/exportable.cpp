
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>
#include <fase2/stdparts.h>

#include <exception>
#include <type_traits>

#include "../extra_parts.h"

#include "../fase_gl_utils.h"

#include "funcs.h"

namespace {

void exportTest(fase::ExportableParts& exportable_app) {
    // export current pipeline.
    auto exported = exportable_app.exportPipe();

    // args = [in_1, in_2,..., in_n, out_1, out_2, ... , out_m]
    // Here, pipeline like form float(float, float) is expected,
    // so vs = [in_1, in_2, out]
    std::vector<fase::Variable> vs(3, std::make_unique<float>());
    *vs[0].getWriter<float>() = 2;
    *vs[1].getWriter<float>() = 6;

    // if form of pipeline is unexpected form, false will be returned.
    if (!exported(vs)) {
        throw std::runtime_error("invalid input/output form!");
    }
    std::cout << "1st result : " << *vs[2].getReader<float>() << std::endl;
    exported(vs);
    std::cout << "2nd result : " << *vs[2].getReader<float>() << std::endl;
    exported(vs);
    std::cout << "3rd result : " << *vs[2].getReader<float>() << std::endl;
    exported(vs);
}

void exportTestWithHard(fase::ExportableParts& exportable_app) {
    // export current pipeline, and wrap to "hard".
    // ToHard<[returns...]>::Pipe<[inputs...]>::Gen([...])
    auto exported = fase::ToHard<float>::Pipe<float, float>::Gen(
            exportable_app.exportPipe());

    auto [res1] = exported(2, 6);

    static_assert(std::is_same_v<std::decay_t<decltype(res1)>, float>,
                  "type of res1 is float.");

    std::cout << "1st result : " << res1 << std::endl;
    auto [res2] = exported(2, 6);
    std::cout << "2nd result : " << res2 << std::endl;
    auto [res3] = exported(2, 6);
    std::cout << "3rd result : " << res3 << std::endl;
    exported.reset();
    auto [res4] = exported(2, 6);
    std::cout << "4th result : " << res4 << std::endl;
}

} // namespace

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::ImGuiEditor, fase::ExportableParts, NFDParts> app;

    struct Counter {
        int count;
        void operator()(float& dst) {
            dst = count++;
        }
    };
    FaseAddUnivFunction(Counter{}, void(float&), ("count"), app);

    app.addOptinalButton(
            "Export pipe test",
            [&] {
                try {
                    exportTest(app);
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
            },
            "Export fucused pipeline as float(float, float), \n"
            "and call exported(2, 6) three times.");

    app.addOptinalButton(
            "Export pipe test (Hard Wraped)",
            [&] {
                try {
                    exportTestWithHard(app);
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
            },
            "Export fucused pipeline as float(float, float), \n"
            "and call exported(2, 6) three times,\n"
            "and reset, and call one more");

    AddNFDButtons(app, app);

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
