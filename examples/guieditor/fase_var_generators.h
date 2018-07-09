#ifndef FASE_VAR_GENERATORS_180709
#define FASE_VAR_GENERATORS_180709

#include <fase.h>

bool ImGuiInputValue(const char* label, int* v) {
    return ImGui::InputInt(label, v);
}

bool ImGuiInputValue(const char* label, float* v) {
    return ImGui::InputFloat(label, v);
}

bool ImGuiInputValue(const char* label, double* v) {
    return ImGui::InputDouble(label, v);
}

template <typename T>
bool ImGuiInputValue(const char* label, T* v) {
    // Cast with int
    int i = static_cast<int>(*v);
    const bool ret = ImGuiInputValue(label, &i);
    *v = static_cast<T>(i);
    return ret;
}

template <typename T, typename F>
static void FaseInstallOne(F& fase) {
    fase.addVarGenerator(T(),
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
}

template <typename F>
static void FaseInstallBasicGuiGenerators(F& fase) {
    FaseInstallOne<int>(fase);
    FaseInstallOne<unsigned int>(fase);
    FaseInstallOne<float>(fase);
    FaseInstallOne<double>(fase);

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
}


#endif /* end of include guard */
