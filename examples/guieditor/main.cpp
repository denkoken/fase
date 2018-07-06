#include <fase.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>


void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

void Print(const int& in) {
    std::cout << in << std::endl;
}

GLFWwindow* InitOpenGL(const std::string& window_title) {

    GLFWwindow* window = nullptr;

    // Set error callback
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;;
        return nullptr;
    }
    atexit([]() {
        glfwTerminate();
    });

    // OpenGL context flags (OpenGL 4.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create hidden OpenGL window and context
    window = glfwCreateWindow(600, 400, window_title.c_str(), NULL, NULL);
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

void InitImGui(GLFWwindow* window) {
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Setup style
    ImGui::StyleColorsDark();
}

int main() {
    // Create OpenGL window
    GLFWwindow *window = InitOpenGL("GUI Editor Example");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window);

    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> fase;

    // Register functions
    FaseAddFunctionBuilder(fase, Add, (const int&, const int&, int&),
                           ("src1", "src2", "dst"), 0, 0, 0);
    FaseAddFunctionBuilder(fase, Square, (const int&, int&), ("src", "dst"), 0,
                           0);
    FaseAddFunctionBuilder(fase, Print, (const int&), ("src"), 0);

//     // Register for parsing command line string
//     fase.addVarGenerator(std::function<int(const std::string&)>(
//             [](const std::string& s) { return std::atoi(s.c_str()); }));
//
    fase.startEditing();

    return 0;
}
