
#include "imgui_editor.h"

#include <cmath>

#include "../utils.h"
#include "imgui_commons.h"

namespace fase {

using std::map, std::string, std::vector;

namespace {

struct OptionalButton {
    std::string name;
    std::function<void()> callback;
    std::string description;
};

bool BeginPopupContext(const char* str, bool condition, int button) {
    if (condition && ImGui::IsMouseReleased(button)) ImGui::OpenPopup(str);
    return ImGui::BeginPopup(str);
}

float GetVolume(const size_t& idx) {
    if (idx == 0)
        return 0.f;
    else if (idx == 1)
        return 1.f;
    else {
        size_t i = size_t(std::pow(2, int(std::log2(idx - 1))));
        return ((idx - i) * 2 - 1) / 2.f / float(i);
    }
}

constexpr float NODE_COL_SCALE = 155.f;
constexpr float NODE_COL_OFFSET = 100.f;

ImU32 GenNodeColor(const size_t& idx) {
    const float v = GetVolume(idx);
    // clang-format off
    float r = (v <= 0.25f) ? 1.f :
              (v <= 0.50f) ? 1.f - (v - 0.25f) / 0.25f : 0.f;
    float g = (v <= 0.25f) ? v / 0.25f :
              (v <= 0.75f) ? 1.f : 1.f - (v - 0.75f) / 0.25f;
    float b = (v <= 0.50f) ? 0.f :
              (v <= 0.75f) ? (v - 0.5f) / 0.25f : 1.f;
    // clang-format on
    r = r * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    g = g * NODE_COL_SCALE * 0.5f + NODE_COL_OFFSET;
    b = b * NODE_COL_SCALE * 1.0f + NODE_COL_OFFSET;
    return IM_COL32(int(r), int(g), int(b), 200);
}

void DrawOptionalButtons(vector<OptionalButton>& optional_buttons,
                         LabelWrapper label) {
    label.addSuffix("##OptionalButton");
    for (auto& ob_data : optional_buttons) {
        if (ImGui::Button(label(ob_data.name))) {
            ob_data.callback();
        }
        ImGui::SameLine();
        ImGui::Text("%s", ob_data.description.c_str());
    }
}

void DrawCanvas(const ImVec2& scroll_pos, float size) {
    const ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
    const ImVec2 win_pos = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_sz = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (float x = std::fmod(scroll_pos.x, size); x < canvas_sz.x; x += size) {
        draw_list->AddLine(ImVec2(x, 0.0f) + win_pos,
                           ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
    }
    for (float y = std::fmod(scroll_pos.y, size); y < canvas_sz.y; y += size) {
        draw_list->AddLine(ImVec2(0.0f, y) + win_pos,
                           ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
    }
}

const ImVec2 NODE_WINDOW_PADDING = ImVec2(8.f, 6.f);

const ImU32 BORDER_COLOR = IM_COL32(100, 100, 100, 255);
const ImU32 BG_NML_COLOR = IM_COL32(60, 60, 60, 255);
const ImU32 BG_ACT_COLOR = IM_COL32(75, 75, 75, 255);

void drawNodeBox(const ImVec2& node_rect_min, const ImVec2& node_size,
                 const bool is_active, const size_t node_id,
                 LabelWrapper label) {
    const ImVec2 node_rect_max = node_rect_min + node_size;
    ImGui::InvisibleButton(label("node"), node_size);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Background
    const ImU32 bg_col = is_active ? BG_ACT_COLOR : BG_NML_COLOR;
    draw_list->AddRectFilled(node_rect_min, node_rect_max, bg_col, 4.f);
    // Border
    draw_list->AddRect(node_rect_min, node_rect_max, BORDER_COLOR, 4.f);
    // Title
    const float line_height = ImGui::GetTextLineHeight();
    const float pad_height = NODE_WINDOW_PADDING.y * 2;
    const ImVec2 node_title_rect_max =
            node_rect_min + ImVec2(node_size.x, line_height + pad_height);
    draw_list->AddRectFilled(node_rect_min, node_title_rect_max,
                             GenNodeColor(node_id), 4.f);
}

void SetGuiStyle() {
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.GrabRounding = style.FrameRounding = 5.0;
    style.FrameBorderSize = 0.3f;
}

}  // namespace

class ImGuiEditor::Impl {
public:
    Impl(ImGuiEditor* parent) : pparent(parent) {}

    void addOptinalButton(std::string&& name, std::function<void()>&& callback,
                          std::string&& description);

    bool runEditing(const std::string& win_title,
                    const std::string& label_suffix);

    bool init();

    bool addVarEditor(std::type_index&& type, VarEditorWraped&& f);

private:
    vector<string> opened_pipelines;
    ImGuiEditor* const pparent;

    // Control Window's variables
    vector<OptionalButton> optional_buttons;
    InputText new_core_name_it{64, ImGuiInputTextFlags_EnterReturnsTrue};

    void drawCanvasPannel(const PipelineAPI& core_api, LabelWrapper label);

    bool drawEditWindow(const string& p_name, const string& win_title,
                        const CoreManager& cm, LabelWrapper label);
    bool drawEditWindows(const string& win_title, LabelWrapper label);

    bool drawControlWindows(const string& win_title, LabelWrapper label);
};

void ImGuiEditor::Impl::addOptinalButton(std::string&& name,
                                         std::function<void()>&& callback,
                                         std::string&& description) {
    optional_buttons.emplace_back(OptionalButton{
            std::move(name), std::move(callback), std::move(description)});
}

void ImGuiEditor::Impl::drawCanvasPannel(const PipelineAPI& core_api,
                                         LabelWrapper label) {
    label.addSuffix("##Canvas");
    ChildRAII raii(label(""));

    DrawCanvas(ImVec2(0, 0), 100);
    const ImVec2 canvas_offset = ImGui::GetCursorScreenPos();
    // Draw node box
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(0);  // Background
    size_t i = 0;
    string hovered = "";
    for (auto& [name, node] : core_api.getNodes()) {
        ImGui::SetCursorScreenPos(canvas_offset + ImVec2(i * 200, 10));
        drawNodeBox(canvas_offset + ImVec2(i * 200, 10), ImVec2(150, 150), true,
                    i, label);
        if (ImGui::IsItemHovered()) {
            hovered = name;
        }
        i++;
    }
    for (auto& [name, node] : core_api.getNodes()) {
        if (BeginPopupContext(label(name + "_context"), hovered == name, 1)) {
            ImGui::Text("%s", name.c_str());
            ImGui::Separator();
            if (ImGui::Selectable("delete")) {
            }
            if (ImGui::Selectable("rename")) {
            }
            ImGui::Separator();
            if (ImGui::Selectable("alocate function")) {
            }
            ImGui::EndPopup();
        }
    }

    if (BeginPopupContext(label("canvas_context"), hovered.empty(), 1)) {
        if (ImGui::Selectable("new node")) {
        }
        ImGui::EndPopup();
    }
}

bool ImGuiEditor::Impl::drawEditWindow(const string& p_name,
                                       const string& win_title,
                                       const CoreManager& cm,
                                       LabelWrapper label) {
    label.addSuffix("##" + p_name);
    bool opened = true;
    if (auto raii = WindowRAII(label(win_title + " : Pipe Edit - " + p_name),
                               &opened)) {
        drawCanvasPannel(cm[p_name], label);
    }

    return opened;
}

bool ImGuiEditor::Impl::drawEditWindows(const string& win_title,
                                        LabelWrapper label) {
    auto [lock, pcm] = pparent->getReader(std::chrono::milliseconds(16));
    if (!lock) {
        return true;
    }

    vector<string> should_be_closeds;
    for (auto& p_name : opened_pipelines) {
        bool opened = drawEditWindow(p_name, win_title, *pcm, label);
        if (!opened) {
            should_be_closeds.emplace_back(p_name);
        }
    }
    for (auto& be_closed : should_be_closeds) {
        erase_all(opened_pipelines, be_closed);
    }
    return true;
}

bool ImGuiEditor::Impl::drawControlWindows(const string& win_title,
                                           LabelWrapper label) {
    label.addSuffix("##ControlWindow");
    if (auto raii = WindowRAII(label(win_title + " : Control Window"), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Open Pipeline : ");
        ImGui::SameLine();
        if (new_core_name_it.draw(label("##core_open_name"))) {
            if (!exists(new_core_name_it.text(), opened_pipelines)) {
                opened_pipelines.emplace_back(new_core_name_it.text());
                new_core_name_it.set("");
            }
        }
        ImGui::Separator();
        ImGui::Spacing();
        DrawOptionalButtons(optional_buttons, label);
    }
    return true;
}

bool ImGuiEditor::Impl::runEditing(const std::string& win_title,
                                   const std::string& label_suffix) {
    LabelWrapper label(label_suffix);
    SetGuiStyle();

    drawControlWindows(win_title, label);
    drawEditWindows(win_title, label);

    return true;
}

bool ImGuiEditor::Impl::init() {
    return true;
}

bool ImGuiEditor::Impl::addVarEditor(std::type_index&& type,
                                     VarEditorWraped&& f) {
    // TODO
    (void)type;
    (void)f;
    return true;
}

// ============================== Pimpl Pattern ================================

ImGuiEditor::ImGuiEditor() : pimpl(std::make_unique<Impl>(this)) {}
ImGuiEditor::~ImGuiEditor() = default;

void ImGuiEditor::addOptinalButton(std::string&& name,
                                   std::function<void()>&& callback,
                                   std::string&& description) {
    pimpl->addOptinalButton(std::move(name), std::move(callback),
                            std::move(description));
}

bool ImGuiEditor::runEditing(const std::string& win_title,
                             const std::string& label_suffix) {
    return pimpl->runEditing(win_title, label_suffix);
}

bool ImGuiEditor::init() {
    return pimpl->init();
}

bool ImGuiEditor::addVarEditor(std::type_index&& type, VarEditorWraped&& f) {
    return pimpl->addVarEditor(std::move(type), std::move(f));
}

}  // namespace fase
