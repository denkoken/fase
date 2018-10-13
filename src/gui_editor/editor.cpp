#include "../editor.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <atomic>
#include <cmath>
#include <mutex>
#include <sstream>
#include <thread>

#include "../core_util.h"
#include "view.h"

namespace fase {

namespace {

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
    const bool ret = ImGui::InputText(label, str_buf, n_str);
    // Back to the value
    (*v) = str_buf;
    return ret;
}

template <class List>
void AddListViewer(
        std::map<const std::type_info*, VarEditorWraped>& var_editors) {
    var_editors[&typeid(List)] =
            [v_editor = var_editors[&typeid(typename List::value_type)]](
                    const char* label, const Variable& a) {
                if (a.getReader<List>()->size() >= LIST_VIEW_MAX) {
                    ImGui::Text("Too long to show : %s", label);
                    return Variable();
                }
                ImGui::BeginGroup();
                size_t i = 0;
                for (auto& v : *a.getReader<List>()) {
                    v_editor(
                            (std::string("[") + std::to_string(i) + "]" + label)
                                    .c_str(),
                            v);
                    i++;
                }
                ImGui::EndGroup();
                return Variable();
            };
}

template <typename T>
void EditorMaker_(
        std::map<const std::type_info*, VarEditorWraped>& var_editors) {
    var_editors[&typeid(T)] = [](const char* label, const Variable& a) {
        T copy = *a.getReader<T>();
        if (ImGuiInputValue(label, &copy)) {
            return Variable(std::move(copy));
        }
        return Variable();
    };
}

template <typename T>
void EditorMaker(
        std::map<const std::type_info*, VarEditorWraped>& var_editors) {
    EditorMaker_<T>(var_editors);
    AddListViewer<std::vector<T>>(var_editors);
    AddListViewer<std::list<T>>(var_editors);
}

void SetUpVarEditors(
        std::map<const std::type_info*, VarEditorWraped>* var_editors) {
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

template <typename T>
inline const T& GetVar(const Issue& issue) {
    return *issue.var.getReader<T>();
}

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(const std::shared_ptr<FaseCore>& core, const TypeUtils& utils)
        : view(*core, utils, var_editors),
          core(core),
          utils(utils),
          shold_stop_loop(false),
          is_running(false) {
        SetUpVarEditors(&var_editors);
        response[CORE_UPDATED_KEY] = true;
    }
    ~Impl();

    bool run(const std::string& win_title, const std::string& label_suffix);

    bool addVarEditor(const std::type_info* p, VarEditorWraped&& f);

private:
    View view;
    std::shared_ptr<FaseCore> core;
    const TypeUtils& utils;

    std::map<std::string, ResultReport> reports;
    std::map<const std::type_info*, VarEditorWraped> var_editors;
    std::map<std::string, Variable> response;

    bool is_updated = true;
    bool same_th_loop = false;
    std::string run_response_id;
    std::string err_message;

    // variables for another thread running.
    FaseCore core_buf;
    std::mutex core_mutex;
    std::mutex buf_issues_mutex;
    std::mutex report_mutex;
    std::atomic_bool multi_running;
    std::atomic_bool shold_stop_loop;
    std::atomic_bool is_running;
    std::vector<Issue> buf_issues;
    std::thread pipeline_thread;

    template <bool Loop>
    void startRunning();

    std::map<std::string, Variable> processIssues(std::vector<Issue>* issues);
};

GUIEditor::Impl::~Impl() {
    // wait pipeline_thread
    if (pipeline_thread.joinable()) {
        shold_stop_loop = true;
        pipeline_thread.join();
    }
}

template <>
void GUIEditor::Impl::startRunning<true>() {
    is_running = true;
    shold_stop_loop = false;
    core_buf = *core;
    err_message = "";
    pipeline_thread = std::thread([this]() {
        while (true) {
            std::map<std::string, ResultReport> ret;
            try {
                ret = core_buf.run();
            } catch (std::exception& e) {
                err_message = e.what();
                break;
            }
            report_mutex.lock();
            reports = ret;
            report_mutex.unlock();

            // update Node arguments
            buf_issues_mutex.lock();
            for (const Issue& issue : buf_issues) {
                if (issue.issue == IssuePattern::SetArgValue) {
                    const SetArgValueInfo& info =
                            GetVar<SetArgValueInfo>(issue);
                    core_buf.setNodeArg(info.node_name, info.arg_idx,
                                        info.arg_repr, info.var);
                }
            }
            buf_issues.clear();
            buf_issues_mutex.unlock();

            // update core
            core_mutex.lock();
            *core = core_buf;
            core_mutex.unlock();
            core_buf.build(multi_running, true);

            // Reduce the speed to about 60 fps
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            if (shold_stop_loop) {
                break;
            }
        }
        is_running = false;
    });
}

template <>
void GUIEditor::Impl::startRunning<false>() {
    is_running = true;
    core_buf = *core;
    err_message = "";
    pipeline_thread = std::thread([this]() {
        core_buf.build(multi_running, true);
        std::map<std::string, ResultReport> ret;
        try {
            ret = core_buf.run();
        } catch (std::exception& e) {
            err_message = e.what();
            is_running = false;
            return;
        }

        report_mutex.lock();
        reports = ret;
        report_mutex.unlock();

        // update Node arguments
        buf_issues_mutex.lock();
        for (const Issue& issue : buf_issues) {
            if (issue.issue == IssuePattern::SetArgValue) {
                const SetArgValueInfo& info = GetVar<SetArgValueInfo>(issue);
                core_buf.setNodeArg(info.node_name, info.arg_idx, info.arg_repr,
                                    info.var);
            }
        }
        buf_issues.clear();
        buf_issues_mutex.unlock();

        // update core
        core_mutex.lock();
        *core = core_buf;
        core_mutex.unlock();

        is_running = false;
    });
}

std::map<std::string, Variable> GUIEditor::Impl::processIssues(
        std::vector<Issue>* issues) {
    std::vector<Issue> remains;
    std::map<std::string, Variable> responses_;

    for (const Issue& issue : *issues) {
        if (issue.issue == IssuePattern::AddNode) {
            const AddNodeInfo& info = GetVar<AddNodeInfo>(issue);
            responses_[issue.id] = core->addNode(info.name, info.func_repr);
        } else if (issue.issue == IssuePattern::DelNode) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->delNode(name);
        } else if (issue.issue == IssuePattern::RenameNode) {
            const RenameNodeInfo& info = GetVar<RenameNodeInfo>(issue);
            responses_[issue.id] =
                    core->renameNode(info.old_name, info.new_name);
        } else if (issue.issue == IssuePattern::SetPriority) {
            const SetPriorityInfo& v = GetVar<SetPriorityInfo>(issue);
            responses_[issue.id] = core->setPriority(v.nodename, v.priority);
        } else if (issue.issue == IssuePattern::AddLink) {
            const auto& info = GetVar<AddLinkInfo>(issue);
            responses_[issue.id] =
                    core->addLink(info.src_nodename, info.src_idx,
                                  info.dst_nodename, info.dst_idx);
        } else if (issue.issue == IssuePattern::DelLink) {
            const auto& info = GetVar<DelLinkInfo>(issue);
            core->delLink(info.src_nodename, info.src_idx);
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::Save) {
            const std::string& filename = GetVar<std::string>(issue);
            responses_[issue.id] = SaveFaseCore(filename, *core, utils);
            continue;
        } else if (issue.issue == IssuePattern::Load) {
            const std::string& filename = GetVar<std::string>(issue);
            responses_[issue.id] = LoadFaseCore(filename, core.get(), utils);
        } else if (issue.issue == IssuePattern::AddInput) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->addInput(name);
        } else if (issue.issue == IssuePattern::AddOutput) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->addOutput(name);
        } else if (issue.issue == IssuePattern::DelInput) {
            const size_t& idx = GetVar<size_t>(issue);
            responses_[issue.id] = core->delInput(idx);
        } else if (issue.issue == IssuePattern::DelOutput) {
            const size_t& idx = GetVar<size_t>(issue);
            responses_[issue.id] = core->delOutput(idx);
        } else if (issue.issue == IssuePattern::SetArgValue) {
            const SetArgValueInfo& info = GetVar<SetArgValueInfo>(issue);
            responses_[issue.id] = core->setNodeArg(
                    info.node_name, info.arg_idx, info.arg_repr, info.var);
        } else if (issue.issue == IssuePattern::SwitchProject) {
            const std::string& name = GetVar<std::string>(issue);
            core->switchProject(name);
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::RenameProject) {
            const std::string& name = GetVar<std::string>(issue);
            core->renameProject(name);
            responses_[issue.id] = true;
        } else {
            remains.emplace_back(std::move(issue));
            continue;
        }

        is_updated = true;
        responses_[CORE_UPDATED_KEY] = true;
    }
    *issues = remains;
    return responses_;
}

bool GUIEditor::Impl::run(const std::string& win_title,
                          const std::string& label_suffix) {
    // lock core_mutex
    core_mutex.lock();

    // Draw GUI and get issues.
    report_mutex.lock();
    std::vector<Issue> issues = view.draw(win_title, label_suffix, response);
    report_mutex.unlock();

    // copy issues for running thread.
    if (is_running) {
        buf_issues_mutex.lock();
        for (const Issue& issue : issues) {
            buf_issues.push_back(issue);
        }
        buf_issues_mutex.unlock();
    }

    // Do issue and make responses.
    response = processIssues(&issues);

    // Do Running Issues.
    for (const Issue& issue : issues) {
        if (issue.issue == IssuePattern::BuildAndRun) {
            BuildAndRunInfo info = GetVar<BuildAndRunInfo>(issue);
            multi_running = info.multi_th_build;
            response[issue.id] = core->build(multi_running, true);
            if (response[issue.id]) {
                response[REPORT_RESPONSE_ID] = &reports;
                if (info.another_th_run) {
                    response[RUNNING_ERROR_RESPONSE_ID] = std::string();
                    startRunning<false>();
                    run_response_id = issue.id;
                } else {
                    reports = core->run();
                }
            }
        } else if (issue.issue == IssuePattern::BuildAndRunLoop) {
            BuildAndRunInfo info = GetVar<BuildAndRunInfo>(issue);
            multi_running = info.multi_th_build;
            response[issue.id] = core->build(multi_running, true);
            if (response[issue.id]) {
                response[REPORT_RESPONSE_ID] = &reports;
                if (info.another_th_run) {
                    response[RUNNING_ERROR_RESPONSE_ID] = std::string();
                    startRunning<true>();
                    run_response_id = issue.id;
                } else {
                    same_th_loop = true;
                    run_response_id = issue.id;
                }
            }
        } else if (issue.issue == IssuePattern::StopRunLoop) {
            run_response_id = "";
            shold_stop_loop = true;
            same_th_loop = false;
        }
    }

    if (same_th_loop) {
        if (is_updated) {
            core->build(multi_running, true);
            is_updated = false;
        }
        reports = core->run();

        response[run_response_id] = true;
        response[REPORT_RESPONSE_ID] = &reports;
    }
    core_mutex.unlock();

    // if pipeline run, set response.
    if (is_running) {
        response[run_response_id] = true;
        response[REPORT_RESPONSE_ID] = &reports;
    }

    // if pipeline running is finished, call join().
    if (!is_running && pipeline_thread.joinable()) {
        pipeline_thread.join();
        if (!err_message.empty()) {
            response[RUNNING_ERROR_RESPONSE_ID] = err_message;
            reports.clear();
            response[REPORT_RESPONSE_ID] = &reports;
        }
    }

    return true;
}

bool GUIEditor::Impl::addVarEditor(const std::type_info* p,
                                   VarEditorWraped&& f) {
    var_editors[p] = std::forward<VarEditorWraped>(f);
    return true;
}

// ------------------------------- pImpl pattern -------------------------------
GUIEditor::GUIEditor(const TypeUtils& utils) : PartsBase(utils) {}
bool GUIEditor::addVarEditor(const std::type_info* p, VarEditorWraped&& f) {
    if (!impl) {
        impl = std::make_unique<GUIEditor::Impl>(getCore(), utils);
    }
    return impl->addVarEditor(p, std::forward<VarEditorWraped>(f));
}
GUIEditor::~GUIEditor() {}
bool GUIEditor::runEditing(const std::string& win_title,
                           const std::string& label_suffix) {
    if (!impl) {
        impl = std::make_unique<GUIEditor::Impl>(getCore(), utils);
    }
    return impl->run(win_title, label_suffix);
}

}  // namespace fase
