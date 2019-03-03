
#ifndef IMGUI_COMMONS_H_20190223
#define IMGUI_COMMONS_H_20190223

#include <imgui.h>
#include <string>
#include <vector>

namespace fase {

// Extend ImGui's operator
inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline ImVec2 operator*(const ImVec2& lhs, const float& rhs) {
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

class WindowRAII {
public:
    WindowRAII(const char* name, bool* p_open = nullptr,
               ImGuiWindowFlags flags = 0) {
        opened = ImGui::Begin(name, p_open, flags);
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

class ChildRAII {
public:
    ChildRAII(const char* name, const ImVec2& size = ImVec2(0, 0),
              bool border = false, ImGuiWindowFlags flags = 0) {
        opened = ImGui::BeginChild(name, size, border, flags);
    }
    ~ChildRAII() {
        ImGui::EndChild();
    }

    operator bool() const noexcept {
        return opened;
    }

private:
    bool opened;
};

class InputText {
public:
    InputText(std::size_t size = 64, ImGuiInputTextFlags flags_ = 0)
        : chars(size), flags(flags_) {}

    bool draw(const char* label) {
        return ImGui::InputText(label, &chars[0], chars.size(), flags);
    }

    std::string text() {
        return {chars.begin(), chars.end()};
    }

    void set(const std::string& str) {
        auto size = std::max(str.size(), chars.size());
        chars.clear();
        chars.resize(size);
        for (std::size_t i = 0; i < str.size(); i++) {
            chars[i] = str[i];
        }
    }

private:
    std::vector<char> chars;
    ImGuiInputTextFlags flags;
};

// Label wrapper for suffix
class LabelWrapper {
public:
    LabelWrapper(const std::string& s) : suffix(s) {}

    void addSuffix(const std::string& s) {
        suffix += s;
    }
    const char* operator()(const std::string& label) {
        last_label = label + suffix;
        return last_label.c_str();
    }

private:
    std::string suffix;
    std::string last_label;  // temporary storage to return char*
};

}  // namespace fase

#endif  // IMGUI_COMMONS_H_20190223
