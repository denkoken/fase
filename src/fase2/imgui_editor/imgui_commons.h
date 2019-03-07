
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
               ImGuiWindowFlags flags = 0)
        : opened(ImGui::Begin(name, p_open, flags)) {}
    WindowRAII(WindowRAII&) = delete;
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
              bool border = false, ImGuiWindowFlags flags = 0)
        : opened(ImGui::BeginChild(name, size, border, flags)) {}
    ChildRAII(ChildRAII&) = delete;
    ~ChildRAII() {
        ImGui::EndChild();
    }

    operator bool() const noexcept {
        return opened;
    }

private:
    bool opened;
};

class PopupRAII {
public:
    PopupRAII(const char* name, ImGuiWindowFlags flags = 0)
        : opened(ImGui::BeginPopup(name, flags)) {}
    PopupRAII(PopupRAII&) = delete;
    ~PopupRAII() {
        if (opened) ImGui::EndPopup();
    }

    operator bool() const noexcept {
        return opened;
    }

private:
    bool opened;
};

class PopupModalRAII {
public:
    PopupModalRAII(const char* name, bool closable,
                   ImGuiWindowFlags flags = 0) {
        if (closable) {
            bool close_f = true;
            opened = ImGui::BeginPopupModal(name, &close_f, flags);
            if (!close_f) {
                ImGui::CloseCurrentPopup();
            }
        } else {
            opened = ImGui::BeginPopupModal(name, nullptr, flags);
        }
    }
    PopupModalRAII(PopupRAII&) = delete;
    ~PopupModalRAII() {
        if (opened) ImGui::EndPopup();
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

class Combo {
public:
    Combo(std::vector<std::string>&& choice_texts_ = {})
        : choice_texts(std::move(choice_texts_)) {}
    bool draw(const char* label) {
        return ImGui::Combo(
                label, &curr_idx,
                [](void* vec, int idx, const char** out_text) {
                    auto& vector = *static_cast<std::vector<std::string>*>(vec);
                    if (idx < 0 || static_cast<int>(vector.size()) <= idx) {
                        return false;
                    } else {
                        *out_text = vector.at(size_t(idx)).c_str();
                        return true;
                    }
                },
                static_cast<void*>(&choice_texts), int(choice_texts.size()));
    }

    void set(std::vector<std::string>&& choice_texts_) {
        choice_texts = std::move(choice_texts_);
    }

    std::string text() {
        return choice_texts[size_t(curr_idx)];
    }
    int id() {
        return curr_idx;
    }

private:
    int curr_idx = 0;
    std::vector<std::string> choice_texts;
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
