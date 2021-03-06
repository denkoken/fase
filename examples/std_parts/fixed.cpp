
#include <fase2/fase.h>
#include <fase2/imgui_editor/imgui_editor.h>
#include <fase2/stdparts.h>

#include <string>
#include <vector>

#include "../extra_parts.h"
#include "../fase_gl_utils.h"

#include "funcs.h"

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::ImGuiEditor, fase::FixedPipelineParts, NFDParts> app;

    std::vector<std::string> in_arg{"red", "green", "blue", "count"};
    std::vector<std::string> out_arg{
            "dst_red",
            "dst_green",
            "dst_blue",
    };

    auto api = app.newPipeline<float, float, float>("fixed", in_arg, out_arg)
                       .fix<float, float, float, float>();

    auto hook = [&](std::vector<float>* bg_col) {
        try {
            if (!api) {
                return; // "fixed" is deleted or something went wrong.
            }
            bg_col->resize(3);
            auto [r, g, b] =
                    api(bg_col->at(0), bg_col->at(1), bg_col->at(2), 0.f);
            r = r > 1.f ? r - 1.f : r;
            g = g > 1.f ? g - 1.f : g;
            b = b > 1.f ? b - 1.f : b;
            bg_col->at(0) = r;
            bg_col->at(1) = g;
            bg_col->at(2) = b;
        } catch (fase::TryToGetEmptyVariable&) {
        }
    };

    AddNFDButtons(app, app);

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, app, hook);

    return 0;
}
