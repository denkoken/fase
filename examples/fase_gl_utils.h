#ifndef FASE_GL_UTILS_H_180709
#define FASE_GL_UTILS_H_180709

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <fase2/imgui_editor/imgui_editor.h>

#include <functional>

static GLFWwindow* InitOpenGL(const std::string& window_title) {
    GLFWwindow* window = nullptr;

    // Set error callback
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        ;
        return nullptr;
    }
    atexit([]() { glfwTerminate(); });

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    // OpenGL context flags (OpenGL 4.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create hidden OpenGL window and context
    window = glfwCreateWindow(1200, 900, window_title.c_str(), NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL context
    if (gl3wInit() != 0) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return nullptr;
    }

    return window;
}

static void InitImGui(GLFWwindow* window, const std::string& font_path) {
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Add extra font
    io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __APPLE__
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    // TODO check
    ImGui_ImplOpenGL3_Init();
#endif

    // Setup style
    ImGui::StyleColorsDark();

    atexit([]() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    });
}

static void
RunRenderingLoop(GLFWwindow* window, fase::ImGuiEditor& editor,
                 std::function<void(std::vector<float>*)> hook = {}) {
    std::vector<float> bg_col = {0.45f, 0.55f, 0.60f};
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

        // Call inserted process
        if (!editor.runEditing("Fase Editor", "##fase")) {
            break;
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        if (hook) {
            hook(&bg_col);
        }

        glClearColor(bg_col[0], bg_col[1], bg_col[2], 1.f);

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }
}

#endif /* end of include guard */
