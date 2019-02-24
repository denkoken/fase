
#ifndef IMGUI_COMMONS_H_20190223
#define IMGUI_COMMONS_H_20190223

#include <imgui.h>
#include <string>

namespace fase {

class WindowRAII {
public:
    WindowRAII(const std::string& name, bool* p_open = nullptr,
               ImGuiWindowFlags flags = 0) {
        opened = ImGui::Begin(name.c_str(), p_open, flags);
    }
    ~WindowRAII() {
        ImGui::End();
    }

    operator bool() const noexcept {
        return opened;
    }

private:
    bool opened;
};

}  // namespace fase

#endif  // IMGUI_COMMONS_H_20190223
