#include "setup_var_editors.h"

#include <list>
#include <vector>

#include <imgui.h>

#include "../utils.h"

namespace fase {

namespace {

using VarEditorWraped = std::function<Variable(const char*, const Variable&)>;

constexpr size_t LIST_VIEW_MAX = 32;

// GUI for <bool>
bool ImGuiInputValue(const char* label, bool* v) {
    return ImGui::Checkbox(label, v);
}

// GUI for <int>
bool ImGuiInputValue(const char* label, int* v,
                     int v_min = std::numeric_limits<int>::min(),
                     int v_max = std::numeric_limits<int>::max()) {
    const float v_speed = 1.0f;
    ImGui::PushItemWidth(100);
    bool ret = ImGui::DragInt(label, v, v_speed, v_min, v_max);
    ImGui::PopItemWidth();
    return ret;
}

// GUI for <float>
bool ImGuiInputValue(const char* label, float* v,
                     float v_min = std::numeric_limits<float>::min(),
                     float v_max = std::numeric_limits<float>::max()) {
    const float v_speed = 0.1f;
    ImGui::PushItemWidth(100);
    bool ret = ImGui::DragFloat(label, v, v_speed, v_min, v_max, "%.4f");
    ImGui::PopItemWidth();
    return ret;
}

// GUI for <int link type>
template <typename T, typename C = int>
bool ImGuiInputValue(const char* label, T* v) {
    C i = static_cast<C>(*v);
    const bool ret = ImGuiInputValue(label, &i, std::numeric_limits<C>::min(),
                                     std::numeric_limits<C>::max());
    *v = static_cast<T>(i);
    return ret;
}

// GUI for <double>
bool ImGuiInputValue(const char* label, double* v) {
    return ImGuiInputValue<double, float>(label, v);
}

// GUI for <std::string>
bool ImGuiInputValue(const char* label, std::string* v) {
    // Copy to editing buffer
    char str_buf[1024];
    const size_t n_str = sizeof(str_buf);
    strncpy(str_buf, (*v).c_str(), n_str);
    // Show GUI
    ImGui::PushItemWidth(200);
    const bool ret = ImGui::InputText(label, str_buf, n_str);
    ImGui::PopItemWidth();
    if (ret) {
        // Back to the value
        (*v) = str_buf;
    }
    return ret;
}

template <class List>
void AddListViewer(std::map<std::type_index, VarEditorWraped>& var_editors) {
    using V_Type = typename List::value_type;
    var_editors[typeid(List)] = [v_editor = var_editors[typeid(V_Type)]](
                                        const char* label, const Variable& a) {
        if (a.getReader<List>()->size() >= LIST_VIEW_MAX) {
            ImGui::Text("Too long to show : %s", split(label, '#')[0].c_str());
            return Variable();
        } else if (a.getReader<List>()->empty()) {
            ImGui::Text("Empty : %s", split(label, '#')[0].c_str());
            return Variable();
        }
        ImGui::BeginGroup();
        size_t i = 0;
        for (auto& v : *a.getReader<List>()) {
            auto&& label_ = std::string("[") + std::to_string(i) + "]" + label;
            v_editor(label_.c_str(), std::make_unique<V_Type>(v));
            i++;
        }
        ImGui::EndGroup();
        return Variable();
    };
}

template <typename T>
void EditorMaker_(std::map<std::type_index, VarEditorWraped>& var_editors) {
    var_editors[typeid(T)] = [](const char* label, const Variable& a) {
        T copy = *a.getReader<T>();
        if (ImGuiInputValue(label, &copy)) {
            return Variable(std::make_unique<T>(std::move(copy)));
        }
        return Variable();
    };
}

template <typename T>
void EditorMaker(std::map<std::type_index, VarEditorWraped>& var_editors) {
    EditorMaker_<T>(var_editors);
    AddListViewer<std::vector<T>>(var_editors);
    AddListViewer<std::list<T>>(var_editors);
}

} // namespace

void SetupDefaultVarEditors(
        std::map<std::type_index, VarEditorWraped>* var_editors) {
    EditorMaker<int>(*var_editors);

    EditorMaker_<bool>(*var_editors);
    // * Character types
    EditorMaker<char>(*var_editors);
    EditorMaker<unsigned char>(*var_editors);
    // * Integer types
    EditorMaker<int>(*var_editors);
    EditorMaker<unsigned int>(*var_editors);
    EditorMaker<short>(*var_editors);
    EditorMaker<unsigned short>(*var_editors);
    EditorMaker<long>(*var_editors);
    EditorMaker<unsigned long>(*var_editors);
    EditorMaker<long long>(*var_editors);
    EditorMaker<unsigned long long>(*var_editors);
    // * Floating types
    EditorMaker<float>(*var_editors);
    EditorMaker<double>(*var_editors);

    // STD containers
    // * string
    EditorMaker<std::string>(*var_editors);
}

} // namespace fase
