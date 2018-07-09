#include <fase.h>

#include "init_gl.h"

void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

void Print(const int& in) {
    std::cout << in << std::endl;
}

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> fase;

    // Register functions
    FaseAddFunctionBuilder(fase, Add, (const int&, const int&, int&),
                           ("in1", "in2", "out"), 0, 0, 0);
    FaseAddFunctionBuilder(fase, Square, (const int&, int&), ("in", "out"), 0,
                           0);
    FaseAddFunctionBuilder(fase, Print, (const int&), ("in"), 0);

    // Register for argument editing
    fase.addVarGenerator(int(),
                         fase::GuiGeneratorFunc([](const char* label,
                                                   const fase::Variable& v,
                                                   std::string& expr) {
                             int* v_p = &*v.getReader<int>();
                             const bool chg = ImGui::InputInt(label, v_p);
                             expr = std::to_string(*v_p);
                             return chg;
                         }));

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Key inputs
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS &&
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            break;
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw Fase's interface
        if (!fase.runEditing("Fase Editor", "##fase")) {
            break;
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    return 0;
}
