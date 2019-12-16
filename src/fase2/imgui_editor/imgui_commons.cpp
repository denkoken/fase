#include "imgui_commons.h"

#include <algorithm>
#include <string>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;

#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#endif

namespace fase {

InputPath::InputPath(Prefferences prefferences_, std::size_t char_size)
    : prefferences(prefferences_), chars(char_size) {
    updatePathBuffer();
}

bool InputPath::draw(const char* label) {
    ImGui::BeginGroup();
    bool is_updated = ImGui::InputText(label, &chars[0], chars.size(), flags);
    if (is_updated) {
        updatePathBuffer();
    }

    int i = 0;
    for (auto& path_buf : path_buffer) {
        if (ImGui::Selectable(path_buf.c_str(), false,
                              ImGuiSelectableFlags_DontClosePopups)) {
            set(path_buf);
            is_updated = true;
        }
        if (i++ >= prefferences.viewing_limit) {
            break;
        }
    }
    ImGui::EndGroup();

    return is_updated;
}

void InputPath::updatePathBuffer() {
#if __cpp_lib_filesystem || __cpp_lib_experimental_filesystem
    fs::path curr = text();
    if (!curr.has_root_directory()) {
        curr = "." / curr;
    }
    std::error_code ec;

    fs::path search_dir =
            fs::is_directory(curr, ec) ? curr : curr.parent_path();

    if (!fs::exists(search_dir, ec)) {
        return;
    }
    auto options = fs::directory_options::skip_permission_denied;

    fs::recursive_directory_iterator rdi(search_dir, options, ec);
    fs::directory_iterator di(search_dir, options, ec);

    if (ec) {
        return;
    }
    path_buffer.clear();
    auto add = [&](const fs::path& path) {
        std::string str = path.string();
        if (!curr.has_root_directory()) {
            str = str.substr(2);
        }
        if (!exists(str, path_buffer) && str != text()) {
            path_buffer.push_back(str);
        }
    };
    const std::vector<std::string>& exts = prefferences.target_extensions;
    int i = 0;
    for (const fs::path& p : rdi) {
        if (exists(p.extension().string(), exts) &&
            p.string().find(curr.string()) == 0) {
            add(p);
        }
        if (i++ >= prefferences.searching_limit) {
            break;
        }
    }
    for (const fs::path& p : di) {
        if (fs::is_directory(p, ec) && p.string().find(curr.string()) == 0) {
            add(p / "");
        }
    }
#endif
}

} // namespace fase
