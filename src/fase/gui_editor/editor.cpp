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

#include "../debug_macros.h"

namespace fase {

namespace guieditor {

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

std::vector<std::string> split(const std::string& str, const char& sp) {
    size_t start = 0;
    size_t end;
    std::vector<std::string> dst;
    while (true) {
        end = str.find_first_of(sp, start);
        dst.emplace_back(std::string(str, start, end - start));
        start = end + 1;
        if (end >= str.size()) {
            break;
        }
    }
    return dst;
}

struct OptionalButton {
    std::string name;
    std::function<void()> call_back;
    std::vector<std::string> descriptions;
};

}  // anonymous namespace

class GUIEditor::Impl {
public:
    Impl(const std::shared_ptr<FaseCore>& core_, const TypeUtils& utils_)
        : core(core_),
          utils(utils_),
          view(*core_, utils_, var_editors),
          shold_stop_loop(false),
          is_running(false) {
        SetUpVarEditors(&var_editors);
    }
    ~Impl();

    bool run(std::mutex& core_mutex, const std::string& win_title,
             const std::string& label_suffix);

    bool addVarEditor(const std::type_info* p, VarEditorWraped&& f);

    void addOptinalButton(std::string&& name, std::function<void()>&& callback,
                          std::string&& description);

private:
    std::shared_ptr<FaseCore> core;
    const TypeUtils& utils;

    View view;

    std::list<FaseCore> back_history;
    std::list<FaseCore> forward_history;

    std::map<std::string, ResultReport> reports;
    std::map<const std::type_info*, VarEditorWraped> var_editors;
    std::map<std::string, Variable> response;

    std::list<OptionalButton> optional_buttons;

    bool same_th_loop = false;
    std::string run_response_id;
    std::string err_message;

    // variables for another thread running.
    FaseCore core_buf;
    std::mutex* pcore_mutex;
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

void GUIEditor::Impl::addOptinalButton(std::string&& name,
                                       std::function<void()>&& callback,
                                       std::string&& description) {
    std::vector<std::string> description_lines = split(description, '\n');
    optional_buttons.emplace_back(
            OptionalButton{std::forward<std::string>(name),
                           std::forward<std::function<void()>>(callback),
                           std::move(description_lines)});
}

template <>
void GUIEditor::Impl::startRunning<true>() {
    is_running = true;
    shold_stop_loop = false;
    core_buf = *core;
    err_message = "";
    core_buf.build(multi_running, true);
    pipeline_thread = std::thread([this]() {
        while (true) {
            std::map<std::string, ResultReport> ret;
            try {      // for catch nested error
                try {  // for catch ErrorThrownByNode.
                    ret = core_buf.run();
                } catch (ErrorThrownByNode& e) {
                    err_message = e.what();
                    e.rethrow_nested();
                }
            } catch (...) {
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
            {
                std::lock_guard<std::mutex> core_guard(*pcore_mutex);
                *core = core_buf;
            }
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
        try {      // for catch nested error
            try {  // for catch ErrorThrownByNode.
                ret = core_buf.run();
            } catch (ErrorThrownByNode& e) {
                err_message = e.what();
                is_running = false;
                e.rethrow_nested();
            }
        } catch (...) {
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
        {
            std::lock_guard<std::mutex> core_guard(*pcore_mutex);
            *core = core_buf;
        }

        is_running = false;
    });
}

std::map<std::string, Variable> GUIEditor::Impl::processIssues(
        std::vector<Issue>* issues) {
    if (issues->empty()) {
        return {};
    }

    std::vector<Issue> remains;
    std::map<std::string, Variable> responses_;

    int privious_version = core->getVersion();
    FaseCore old_version_core = *core;

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
        } else if (issue.issue == IssuePattern::SwitchPipeline) {
            const std::string& name = GetVar<std::string>(issue);
            core->switchPipeline(name);
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::RenamePipeline) {
            const std::string& name = GetVar<std::string>(issue);
            core->renamePipeline(name);
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::MakeSubPipeline) {
            const std::string& name = GetVar<std::string>(issue);
            responses_[issue.id] = core->makeSubPipeline(name);
        } else if (issue.issue == IssuePattern::ImportSubPipeline) {
            const ImportSubPipelineInfo& info =
                    GetVar<ImportSubPipelineInfo>(issue);
            responses_[issue.id] = ImportSubPipeline(info.filename, core.get(),
                                                     utils, info.target_name);
        } else {
            remains.emplace_back(std::move(issue));
        }
    }

    if (privious_version < core->getVersion()) {  // version up!
        // restore privious version core.
        back_history.emplace_back(old_version_core);
        forward_history.clear();
    }

    for (const Issue& issue : remains) {
        if (issue.issue == IssuePattern::Undo) {
            if (back_history.empty()) {
                responses_[issue.id] = false;
                continue;
            }
            forward_history.emplace_back(std::move(*core));
            *core = std::move(back_history.back());
            back_history.pop_back();
            responses_[issue.id] = true;
        } else if (issue.issue == IssuePattern::Redo) {
            if (forward_history.empty()) {
                responses_[issue.id] = false;
                continue;
            }
            back_history.emplace_back(std::move(*core));
            *core = std::move(forward_history.back());
            forward_history.pop_back();
            responses_[issue.id] = true;
        }
    }

    *issues = remains;

    return responses_;
}

bool GUIEditor::Impl::run(std::mutex& core_mutex, const std::string& win_title,
                          const std::string& label_suffix) {
    pcore_mutex = &core_mutex;
    {  // touching the core.
        std::lock_guard<std::mutex> core_guard(*pcore_mutex);

        // Draw GUI and get issues.
        report_mutex.lock();
        std::vector<Issue> issues =
                view.draw(win_title, label_suffix, response);
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
                bool build_sucesses = core->build(multi_running, true);
                response[issue.id] = build_sucesses;
                if (build_sucesses) {
                    response[REPORT_RESPONSE_ID] = &reports;
                    if (info.another_th_run) {
                        response[RUNNING_ERROR_RESPONSE_ID] = std::string();
                        startRunning<false>();
                        run_response_id = issue.id;
                    } else {
                        reports = core->run();
                    }
                } else {
                    response[RUNNING_ERROR_RESPONSE_ID] =
                            std::string("Build failed.");
                }
            } else if (issue.issue == IssuePattern::BuildAndRunLoop) {
                BuildAndRunInfo info = GetVar<BuildAndRunInfo>(issue);
                multi_running = info.multi_th_build;
                bool build_sucesses = core->build(multi_running, true);
                response[issue.id] = build_sucesses;
                if (build_sucesses) {
                    response[REPORT_RESPONSE_ID] = &reports;
                    if (info.another_th_run) {
                        response[RUNNING_ERROR_RESPONSE_ID] = std::string();
                        startRunning<true>();
                        run_response_id = issue.id;
                    } else {
                        same_th_loop = true;
                        run_response_id = issue.id;
                    }
                } else {
                    response[RUNNING_ERROR_RESPONSE_ID] =
                            std::string("Build failed.");
                }
            } else if (issue.issue == IssuePattern::StopRunLoop) {
                run_response_id = "";
                shold_stop_loop = true;
                same_th_loop = false;
            }
        }

        if (same_th_loop) {
            core->build(multi_running, true);
            reports = core->run();

            response[run_response_id] = true;
            response[REPORT_RESPONSE_ID] = &reports;
        }
    }  // finish touching the core.

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

    // Optional Button Window.
    if (!optional_buttons.empty()) {
        ImGui::Begin(
                ("Optional Trigger Buttons" + ("##" + label_suffix)).c_str());

        for (OptionalButton& ob : optional_buttons) {
            if (ImGui::Button(ob.name.c_str())) {
                ob.call_back();
            }
            ImGui::SameLine();
            ImGui::Text(" : ");
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (std::string& line : ob.descriptions) {
                ImGui::Text("%s", line.c_str());
            }
            ImGui::EndGroup();
        }

        ImGui::End();
    }

    return true;
}

bool GUIEditor::Impl::addVarEditor(const std::type_info* p,
                                   VarEditorWraped&& f) {
    var_editors[p] = std::forward<VarEditorWraped>(f);
    return true;
}

// ------------------------------- pImpl pattern -------------------------------
GUIEditor::GUIEditor(const TypeUtils& utils_) : PartsBase(utils_) {}
GUIEditor::~GUIEditor() {}

bool GUIEditor::init() {
    impl = std::make_unique<GUIEditor::Impl>(getCore(), utils);
    return true;
}

bool GUIEditor::addVarEditor(const std::type_info* p, VarEditorWraped&& f) {
    return impl->addVarEditor(p, std::forward<VarEditorWraped>(f));
}

void GUIEditor::addOptinalButton(std::string&& name,
                                 std::function<void()>&& callback,
                                 std::string&& description) {
    impl->addOptinalButton(std::forward<std::string>(name),
                           std::forward<std::function<void()>>(callback),
                           std::forward<std::string>(description));
}

bool GUIEditor::runEditing(const std::string& win_title,
                           const std::string& label_suffix) {
    return impl->run(core_mutex, win_title, label_suffix);
}

}  // namespace guieditor

}  // namespace fase
