#ifndef FASE_VAR_GENERATORS_180709
#define FASE_VAR_GENERATORS_180709

#include <fase.h>

// GUI for <bool>
bool ImGuiInputValue(const char* label, bool* v) {
    return ImGui::Checkbox(label, v);
}

// GUI for <int>
bool ImGuiInputValue(const char* label, int* v,
                     int v_min = std::numeric_limits<int>::min(),
                     int v_max = std::numeric_limits<int>::max()) {
    const float v_speed = 1.0f;
    return ImGui::DragInt(label, v, v_speed, v_min, v_max);
}

// GUI for <float>
bool ImGuiInputValue(const char* label, float* v,
                     float v_min = std::numeric_limits<float>::min(),
                     float v_max = std::numeric_limits<float>::max()) {
    const float v_speed = 0.1f;
    return ImGui::DragFloat(label, v, v_speed, v_min, v_max, "%.4f");
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


///
/// Install one GUI for a type 'T'
///
template <typename T, typename F>
static void FaseInstallOne(F& fase) {
    const bool ret = fase.addVarGenerator(T(),
                         fase::GuiGeneratorFunc([](const char* label,
                                                   const fase::Variable& v,
                                                   std::string& expr) {
                             T& val = *v.getReader<T>();
                             const bool chg = ImGuiInputValue(label, &val);
                             if (chg) {
                                 expr = std::to_string(val);
                             }
                             return chg;
                         }));
    if (!ret) {
        std::cerr << "Failed to install Fase GUIEditor's interface for '"
                  << typeid(T).name() << "'" << std::endl;
    }
}

///
/// Install Fase GUIEditor's interfaces for basic types (primitives and std)
///
template <typename F>
static void FaseInstallBasicGuiGenerators(F& fase) {
    // Primitives [https://en.cppreference.com/w/cpp/language/types]
    // * Boolean type
    FaseInstallOne<bool>(fase);
    // * Character types
    FaseInstallOne<char>(fase);
    FaseInstallOne<unsigned char>(fase);
    // * Integer types
    FaseInstallOne<int>(fase);
    FaseInstallOne<unsigned int>(fase);
    FaseInstallOne<short>(fase);
    FaseInstallOne<unsigned short>(fase);
    FaseInstallOne<long>(fase);
    FaseInstallOne<unsigned long>(fase);
    FaseInstallOne<long long>(fase);
    FaseInstallOne<unsigned long long>(fase);
    // * Floating types
    FaseInstallOne<float>(fase);
    FaseInstallOne<double>(fase);

    // STD containers
    // * string
    fase.addVarGenerator(
            std::string(), fase::GuiGeneratorFunc([&](const char* label,
                                                      const fase::Variable& v,
                                                      std::string& expr) {
                // Get editing value
                std::string& str = *v.getReader<std::string>();
                // Copy to editing buffer
                char str_buf[1024];
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
}


#endif /* end of include guard */
