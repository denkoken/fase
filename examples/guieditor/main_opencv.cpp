#include <fase/editor.h>
#include <fase/fase.h>

#include "fase_gl_utils.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <random>

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

void Random(int& v, const int& min_v, const int& max_v) {
    static std::random_device rnd;
    const int range = max_v - min_v;
    if (range <= 0) {
        v = min_v;
    } else {
        v = static_cast<int>(rnd()) % (range) + min_v;
        if (v < min_v) {
            v += range;
        }
    }
}

int main() {
    // Create Fase instance with GUI editor
    fase::Fase<fase::GUIEditor> app;

    // Register functions
    FaseAddFunctionBuilder(app, LoadImage, (const std::string&, cv::Mat&),
                           ("filename", "img"));
    FaseAddFunctionBuilder(app, BlurImage, (const cv::Mat&, cv::Mat&, int),
                           ("in", "out", "ksize"), cv::Mat(), cv::Mat(), 3);
    FaseAddFunctionBuilder(app, Random, (int&, const int&, const int&),
                           ("out", "min", "max"), 0, 0, 256);

    //   <cv::Mat>
    std::map<std::string, std::tuple<GLuint, int, int>> tex_ids;
    app.addVarEditor<cv::Mat>([&](const char* label, const cv::Mat& img)
                                      -> std::unique_ptr<cv::Mat> {
        if (img.empty()) {
            ImGui::Text("Empty");
            return {};
        }
        if (img.type() != CV_8UC3 || img.channels() != 3) {
            ImGui::Text("Not uint8 BGR image");
            return {};
        }
        const bool exist = tex_ids.count(label);
        auto& ctr = tex_ids[label];
        GLuint& id = std::get<0>(ctr);
        int& width = std::get<1>(ctr);
        int& height = std::get<2>(ctr);
        if (!exist || width != img.cols || height != img.rows) {
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // for 3 ch
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0,
                         GL_BGR, GL_UNSIGNED_BYTE, img.data);
            width = img.cols;
            height = img.rows;
        } else {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.cols, img.rows, GL_BGR,
                            GL_UNSIGNED_BYTE, img.data);
        }
        const ImTextureID tex_id =
                reinterpret_cast<void*>(static_cast<intptr_t>(id));
        ImGui::Image(tex_id, ImVec2(200.f, 200.f * img.rows / img.cols),
                     ImVec2(0, 0), ImVec2(1, 1));
        return {};
    });

    FaseRegisterTestIO(
            app, cv::Mat, [](const cv::Mat&) -> std::string { return {}; },
            [](const std::string&) -> cv::Mat { return {}; },
            [](const cv::Mat&) -> std::string { return "cv::Mat()"; });

    // Create OpenGL window
    GLFWwindow* window = InitOpenGL("GUI Editor Example with OpenCV");
    if (!window) {
        return 0;
    }

    // Initialize ImGui
    InitImGui(window, "../third_party/imgui/misc/fonts/Cousine-Regular.ttf");

    // Start main loop
    RunRenderingLoop(window, app);

    return 0;
}
