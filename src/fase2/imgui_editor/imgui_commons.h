
#ifndef IMGUI_COMMONS_H_20190223
#define IMGUI_COMMONS_H_20190223

#include <cmath>
#include <deque>
#include <functional>
#include <string>
#include <vector>

#include <imgui.h>

#include "../manager.h"

namespace fase {

struct GuiNode {
    ImVec2 pos;
    ImVec2 size;
    std::vector<ImVec2> arg_poses;
    std::vector<char> arg_inp_hovered;
    std::vector<char> arg_out_hovered;
    std::size_t id = std::size_t(-1);

    std::size_t arg_size() const {
        return arg_poses.size();
    }

    void alloc(std::size_t n_args) {
        arg_poses.resize(n_args);
        arg_inp_hovered.resize(n_args);
        arg_out_hovered.resize(n_args);
    }

    ImVec2 getInputSlot(const std::size_t idx) const {
        return arg_poses[idx];
    }
    ImVec2 getOutputSlot(const std::size_t idx) const {
        return ImVec2(arg_poses[idx].x + size.x, arg_poses[idx].y);
    }
};

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
        return {chars.begin(), std::find(chars.begin(), chars.end(), '\0')};
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

class InputPath {
public:
    struct Prefferences {
        int searching_limit = 10e3;
        int viewing_limit = 15;
        std::vector<std::string> target_extensions; // ".txt", "", ".JSON" ...
    };

    InputPath(Prefferences prefferences_, std::size_t char_size = 256);

    bool draw(const char* label);

    std::string text() const {
        return {chars.begin(), std::find(chars.begin(), chars.end(), '\0')};
    }

    void set(const std::string& str) {
        auto size = std::max(str.size() + 1, chars.size());
        chars.resize(size);

        str.copy(&chars[0], str.size());
        chars[str.size()] = '\0';
        updatePathBuffer();
    }

private:
    Prefferences prefferences;
    std::vector<char> chars;
    std::vector<std::string> path_buffer;
    const static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll;

    void updatePathBuffer();
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
                        *out_text = vector.at(std::size_t(idx)).c_str();
                        return true;
                    }
                },
                static_cast<void*>(&choice_texts), int(choice_texts.size()));
    }

    void set(std::vector<std::string>&& choice_texts_) {
        choice_texts = std::move(choice_texts_);
    }

    std::string text() {
        if (choice_texts.empty()) return {};
        return choice_texts[std::size_t(curr_idx)];
    }
    int id() {
        if (choice_texts.empty()) return -1;
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
    std::string last_label; // temporary storage to return char*
};

using Issues = std::deque<std::function<void(CoreManager*)>>;

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
inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y);
}
inline float Length(const ImVec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}
inline ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs) {
    return ImVec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}

ImU32 GenNodeColor(const std::size_t& idx);

inline bool GetIsKeyPressed(const ImGuiKey& key) {
    return ImGui::IsWindowFocused() &&
           ImGui::IsKeyPressed(ImGui::GetKeyIndex(key));
}

inline bool GetIsKeyPressed(char key, bool ctrl = false, bool shift = false,
                            bool alt = false, bool super = false) {
    auto& io = ImGui::GetIO();

    bool s_keys = !ctrl || (ctrl && io.KeyCtrl);
    s_keys &= !shift || (shift && io.KeyShift);
    s_keys &= !alt || (alt && io.KeyAlt);
    s_keys &= !super || (super && io.KeySuper);

    return ImGui::IsWindowFocused() &&
           ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A) + key - 'a') &&
           s_keys;
}

inline bool GetIsKeyDown(const ImGuiKey& key) {
    return ImGui::IsWindowFocused() &&
           ImGui::IsKeyDown(ImGui::GetKeyIndex(key));
}

inline bool GetIsKeyDown(char key, bool ctrl = false, bool shift = false,
                         bool alt = false, bool super = false) {
    auto& io = ImGui::GetIO();

    bool s_keys = !ctrl || (ctrl && io.KeyCtrl);
    s_keys &= !shift || (shift && io.KeyShift);
    s_keys &= !alt || (alt && io.KeyAlt);
    s_keys &= !super || (super && io.KeySuper);

    return ImGui::IsWindowFocused() &&
           ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A) + key - 'a') &&
           s_keys;
}

inline PopupRAII BeginPopupContext(const char* str, bool condition,
                                   int button) {
    if (condition && ImGui::IsWindowHovered() && ImGui::IsMouseReleased(button))
        ImGui::OpenPopup(str);
    return {str};
}

inline PopupModalRAII BeginPopupModal(
        const char* str, bool condition, bool closable = true,
        ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize) {
    if (condition) ImGui::OpenPopup(str);
    return {str, closable, flags};
}

}  // namespace fase

#endif // IMGUI_COMMONS_H_20190223
