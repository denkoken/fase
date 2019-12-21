
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>
#include <fase2/stdparts.h>

#include <string>
#include <vector>

#include "../extra_parts.h"
#include "../fase_gl_utils.h"

#include "funcs.h"

namespace {

void HardCallableTest(fase::HardCallableParts<float>& app) {
    try {
        // Return type have set at makeing app.
        // Here it is float.
        auto [dst] = app.callHard(3.f, 4.f);
        static_assert(std::is_same_v<std::decay_t<decltype(dst)>, float>,
                      "type of dst is float");
        std::cout << "app.callHard(3.f, 4.f) : " << dst << std::endl;
    } catch (std::exception& e) {
        // If input/output form of pipe is different, exception is thrown.
        std::cout << e.what() << std::endl;
        std::cout << " : It almost print \"HardCallableParts : "
                     "input/output type "
                     "isn't match!\"."
                  << std::endl;
    }
}

} // namespace

int main() {
    // Create Fase instance with GUI editor and HardCallableParts.
    // Return type of HardCallableParts should be set when make Fase instance.
    fase::Fase<fase::ImGuiEditor, fase::HardCallableParts<float>, NFDParts> app;

    app.addOptinalButton(
            "HardCallableParts test", [&] { HardCallableTest(app); },
            "call the current pipeline with input=(3.f, 4.f) and "
            "output=[float].");

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
