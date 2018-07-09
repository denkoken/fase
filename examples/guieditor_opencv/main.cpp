#include <fase.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "../guieditor/init_gl.h"

void LoadImage(const std::string& filename, cv::Mat& img) {
    img = cv::imread(filename);
}

void BlurImage(const cv::Mat& src, cv::Mat& dst, int ksize) {
    if (src.empty()) {
        return;  // Avoid assertion by OpenCV
    }
    ksize = std::max(ksize, 1);
    cv::blur(src, dst, cv::Size(ksize, ksize));
}

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> fase;

    // Register functions
    FaseAddFunctionBuilder(fase, LoadImage, (const std::string&, cv::Mat&),
                           ("filename", "img"));
    FaseAddFunctionBuilder(fase, BlurImage, (const cv::Mat&, cv::Mat&, int),
                           ("in", "out", "ksize"), cv::Mat(), cv::Mat(), 3);

    // Register for argument editing
    //   <int>
    fase.addVarGenerator(int(),
                         fase::GuiGeneratorFunc([](const char* label,
                                                   const fase::Variable& v,
                                                   std::string& expr) {
                             int* v_p = &*v.getReader<int>();
                             const bool chg = ImGui::InputInt(label, v_p);
                             expr = std::to_string(*v_p);
                             return chg;
                         }));
    //   <std::string>
    char str_buf[1024];
    fase.addVarGenerator(
            std::string(), fase::GuiGeneratorFunc([&](const char* label,
                                                      const fase::Variable& v,
                                                      std::string& expr) {
                // Get editing value
                std::string& str = *v.getReader<std::string>();
                // Copy to editing buffer
                const size_t n_str = sizeof(str_buf);
                strncpy(str_buf, str.c_str(), n_str);
                // Show GUI
                const bool ret = ImGui::InputText(label, str_buf, n_str);
                // Back to the value
                str = str_buf;
                // Create expression when editing occurs
                if (ret) {
                    expr = "\"" + str + "\"";
                }
                return ret;
            }));
    //   <cv::Mat>
    std::map<std::string, GLuint> tex_ids;
    fase.addVarGenerator(
            cv::Mat(), fase::GuiGeneratorFunc([&](const char* label,
                                                  const fase::Variable& v,
                                                  std::string& expr) {
                (void)expr;
                cv::Mat& img = *v.getReader<cv::Mat>();
                if (img.empty()) {
                    ImGui::Text("Empty");
                    return false;
                }
                if (img.type() != CV_8UC3 || img.channels() != 3) {
                    ImGui::Text("Not uint8 BGR image");
                    return false;
                }
                const bool exist = tex_ids.count(label);
                GLuint& id = tex_ids[label];
                if (!exist) {
                    glGenTextures(1, &id);
                    glBindTexture(GL_TEXTURE_2D, id);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // for 3 ch
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols,
                                 img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE,
                                 img.data);
                } else {
                    glBindTexture(GL_TEXTURE_2D, id);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols,
                                    img.rows, GL_BGR, GL_UNSIGNED_BYTE,
                                    img.data);
                }
                ImTextureID tex_id =
                        reinterpret_cast<void*>(static_cast<intptr_t>(id));
                ImGui::Image(tex_id, ImVec2(200.f, 200.f * img.rows / img.cols),
                             ImVec2(0, 0), ImVec2(1, 1));
                return false;
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
