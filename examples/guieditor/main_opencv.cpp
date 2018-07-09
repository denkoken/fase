#include <fase.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "fase_gl_utils.h"
#include "fase_var_generators.h"

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
    FaseInstallBasicGuiGenerators(fase);
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
    GLFWwindow* window = InitOpenGL("GUI Editor Example with OpenCV");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, [&]() {
        // Draw Fase's interface
        return fase.runEditing("Fase Editor", "##fase");
    });

    return 0;
}
