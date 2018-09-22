#include "../editor.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <sstream>
#include <thread>

#include "../core_util.h"
#include "view.h"

namespace fase {

namespace {

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
    const bool ret = ImGui::InputText(label, str_buf, n_str);
    // Back to the value
    (*v) = str_buf;
    return ret;
}

template <typename T>
void EditorMaker(std::map<const std::type_info*, VarEditor>& var_editors) {
    var_editors[&typeid(T)] = [](const char* label, const Variable& a) {
        T copy = *a.getReader<T>();
        if (ImGuiInputValue(label, &copy)) {
            return Variable(std::move(copy));
        }
        return Variable();
    };
}

void SetUpVarEditors(std::map<const std::type_info*, VarEditor>* var_editors) {
    EditorMaker<int>(*var_editors);

    EditorMaker<bool>(*var_editors);
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

template <typename T>
inline const T& GetVar(const Issue& issue) {
    return *issue.var.getReader<T>();
}

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(FaseCore* core, const TypeUtils& utils)
        : view(*core, utils, var_editors),
          core(core),
          utils(utils),
          is_running(false)
#if 0
          , is_running_loop(false),
          shold_stop_loop(false)
#endif
    {
        SetUpVarEditors(&var_editors);
    }

    bool run(const std::string& win_title, const std::string& label_suffix);

    bool addVarEditor(const std::type_info* p, VarEditor&& f);

private:
    View view;
    FaseCore* core;
    const TypeUtils& utils;

    std::mutex report_mutex;
    std::map<std::string, ResultReport> reports;
    std::map<const std::type_info*, VarEditor> var_editors;
    std::map<std::string, Variable> response;

    bool multi_running;
    bool is_core_updeted = false;
    std::thread pipeline_thread;
    std::string run_looping_id;
    std::atomic_bool is_running;

#if 0
    // variables for loop
    std::mutex core_mutex;
    FaseCore core_buf;
    std::mutex buf_issues_mutex;
    std::vector<Issue> buf_issues;
    std::atomic_bool is_running_loop;
    std::atomic_bool shold_stop_loop;
#endif

    template <bool Loop>
    void startRunning();
    bool isCoreUpdatable();

    std::map<std::string, Variable> processIssues(std::vector<Issue>* issues);
};

template <>
void GUIEditor::Impl::startRunning<true>() {
#if 0
    is_running_loop = true;
    shold_stop_loop = false;
    core_buf = *core;
    pipeline_thread = std::thread([this]() {
        while (true) {
            auto ret = core_buf.run();
            report_mutex.lock();
            reports = ret;
            report_mutex.unlock();

            // update Node arguments
            buf_issues_mutex.lock();
            for (const Issue& issue : buf_issues) {
                if (issue.issue == IssuePattern::SetArgValue) {
                    const SetArgValueInfo& info = GetVar<SetArgValueInfo>(issue);
                    core_buf.setNodeArg(info.node_name, info.arg_idx,
                                        info.arg_repr, info.var);
                }
            }
            buf_issues_mutex.unlock();

            // update core
            core_mutex.lock();
            *core = core_buf;
            core_mutex.unlock();

            if (shold_stop_loop) {
                break;
            }
        }
        is_running_loop = false;
    });
#endif
}

template <>
void GUIEditor::Impl::startRunning<false>() {
    is_running = true;
    pipeline_thread = std::thread([this]() {
        auto ret = core->run();
        report_mutex.lock();
        reports = ret;
        report_mutex.unlock();
        is_running = false;
    });
}

bool GUIEditor::Impl::isCoreUpdatable() {
    if (is_running) return false;
    is_core_updeted = true;
    return true;
}

std::map<std::string, Variable> GUIEditor::Impl::processIssues(
        std::vector<Issue>* issues) {
    std::vector<Issue> remains;
    std::map<std::string, Variable> responses_;

    for (const Issue& issue : *issues) {
        if (issue.issue == IssuePattern::AddNode) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const AddNodeInfo& info = GetVar<AddNodeInfo>(issue);
            responses_[issue.id] = core->addNode(info.name, info.func_repr);
        } else if (issue.issue == IssuePattern::DelNode) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->delNode(name);
        } else if (issue.issue == IssuePattern::RenameNode) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const RenameNodeInfo& info = GetVar<RenameNodeInfo>(issue);
            responses_[issue.id] =
                    core->renameNode(info.old_name, info.new_name);
        } else if (issue.issue == IssuePattern::SetPriority) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const SetPriorityInfo& v = GetVar<SetPriorityInfo>(issue);
            responses_[issue.id] = core->setPriority(v.nodename, v.priority);
        } else if (issue.issue == IssuePattern::AddLink) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const auto& info = GetVar<AddLinkInfo>(issue);
            responses_[issue.id] =
                    core->addLink(info.src_nodename, info.src_idx,
                                  info.dst_nodename, info.dst_idx);
        } else if (issue.issue == IssuePattern::DelLink) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const auto& info = GetVar<DelLinkInfo>(issue);
            core->delLink(info.src_nodename, info.src_idx);
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::Save) {
            const std::string& filename = GetVar<std::string>(issue);
            responses_[issue.id] = SaveFaseCore(filename, *core, utils);
        } else if (issue.issue == IssuePattern::Load) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const std::string& filename = GetVar<std::string>(issue);
            responses_[issue.id] = LoadFaseCore(filename, core, utils);
        } else if (issue.issue == IssuePattern::AddInput) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->addInput(name);
        } else if (issue.issue == IssuePattern::AddOutput) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->addOutput(name);
        } else if (issue.issue == IssuePattern::DelInput) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const size_t& idx = GetVar<size_t>(issue);
            responses_[issue.id] = core->delInput(idx);
        } else if (issue.issue == IssuePattern::DelOutput) {
            const size_t& idx = GetVar<size_t>(issue);
            responses_[issue.id] = core->delOutput(idx);
        } else if (issue.issue == IssuePattern::SetArgValue) {
            if (!isCoreUpdatable()) {
                remains.emplace_back(std::move(issue));
                continue;
            }
            const SetArgValueInfo& info = GetVar<SetArgValueInfo>(issue);
            responses_[issue.id] = core->setNodeArg(
                    info.node_name, info.arg_idx, info.arg_repr, info.var);
        }
        // TODO
        else {
            remains.emplace_back(std::move(issue));
        }
    }
    *issues = remains;
    return responses_;
}

bool GUIEditor::Impl::run(const std::string& win_title,
                          const std::string& label_suffix) {
    // Draw GUI and get issues.
    report_mutex.lock();
    std::vector<Issue> issues = view.draw(win_title, label_suffix, response);
    report_mutex.unlock();

#if 0
    if (is_running_loop) {
        buf_issues_mutex.lock();
        for (const Issue& issue : issues) {
            buf_issues.push_back(issue);
        }
        buf_issues_mutex.unlock();
    }
#endif
    // Do issue and make responses.
    response = processIssues(&issues);

    // Do Running Issues.
    for (const Issue& issue : issues) {
        if (issue.issue == IssuePattern::BuildAndRun) {
            multi_running = GetVar<bool>(issue);
            response[issue.id] = core->build(multi_running, true);
            response[REPORT_RESPONSE_ID] = &reports;
            startRunning<false>();
        } else if (issue.issue == IssuePattern::BuildAndRunLoop) {
            multi_running = GetVar<bool>(issue);
            response[issue.id] = core->build(multi_running, true);
#if 0
            startRunning<true>();
#endif
            run_looping_id = issue.id;
        } else if (issue.issue == IssuePattern::StopRunLoop) {
            run_looping_id = "";
#if 0
            shold_stop_loop = true;
#endif
        }
    }

#if 0
    if (is_running_loop) {
        response[run_looping_id] = true;
        response[REPORT_RESPONSE_ID] = &reports;
    }
#else
    if (!run_looping_id.empty()) {
        if (is_core_updeted) {
            core->build(multi_running, true);
        }
        reports = core->run();
        response[run_looping_id] = true;
        response[REPORT_RESPONSE_ID] = &reports;
        is_core_updeted = false;
    }
#endif
    if (!is_running && pipeline_thread.joinable()) {
        pipeline_thread.join();
    }

    return true;
}

bool GUIEditor::Impl::addVarEditor(const std::type_info* p, VarEditor&& f) {
    var_editors[p] = std::forward<VarEditor>(f);
    return true;
}

// ------------------------------- pImpl pattern -------------------------------
GUIEditor::GUIEditor(FaseCore* core, const TypeUtils& utils)
    : impl(new GUIEditor::Impl(core, utils)) {}
bool GUIEditor::addVarEditor(const std::type_info* p, VarEditor&& f) {
    return impl->addVarEditor(p, std::forward<VarEditor>(f));
}
GUIEditor::~GUIEditor() {}
bool GUIEditor::run(const std::string& win_title,
                    const std::string& label_suffix) {
    return impl->run(win_title, label_suffix);
}

}  // namespace fase
