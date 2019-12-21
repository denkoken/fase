
#ifndef EXTRA_PARTS_H_20190329
#define EXTRA_PARTS_H_20190329

#include <fase2/imgui_editor/imgui_editor.h>

class NFDParts;
void AddNFDButtons(fase::ImGuiEditor&, NFDParts&);

#ifdef USE_NFD

#include <string>

#include <fase2/fase.h>

#include <nfd.h>

class NFDParts : public fase::PartsBase {
public:
    void loadPipelineWithNFD() {
        std::string path = getLoadPathWithNFD();
        if (path.empty()) {
            return;
        }

        auto [guard, pcm] = getAPI()->getWriter(std::chrono::seconds(3));
        if (!pcm) {
            std::clog << "NFDParts::getWriter is timeout." << std::endl;
            return;
        }

        if (!LoadPipelineFromFile(path, pcm.get(),
                                  getAPI()->getConverterMap())) {
            std::cerr << "Loading pipeline from " << path << " failed."
                      << std::endl;
        }
    }

    void savePipelineWithNFD() {
        auto pipe_name =
                std::get<1>(getAPI()->getReader(std::chrono::seconds(3)))
                        ->getFocusedPipeline();
        std::string path = getSavePathWithNFD(pipe_name);
        if (path.empty()) {
            return;
        }

        auto [guard, pcm] = getAPI()->getReader(std::chrono::seconds(3));
        if (!pcm) {
            std::clog << "NFDParts::getReader is timeout." << std::endl;
            return;
        }

        if (!SavePipeline(pipe_name, *pcm, path, getAPI()->getConverterMap())) {
            std::cerr << "Saving pipeline to " << path << " failed."
                      << std::endl;
        }
    }

private:
    template <typename NFD_DIALOG>
    std::string getPathWithNFDTmep(NFD_DIALOG nfd_dialog,
                                   const nfdchar_t* filterList = NULL,
                                   const nfdchar_t* defaultPath = NULL) {
        nfdchar_t* outPath = NULL;
        nfdresult_t result = nfd_dialog(filterList, defaultPath, &outPath);

        if (result == NFD_CANCEL) {
            return {};
        } else if (result != NFD_OKAY) {
            printf("NFD Error: %s\n", NFD_GetError());
            return {};
        }

        std::stringstream ss;
        ss << outPath;

        free(outPath);

        return ss.str();
    }

    std::string getLoadPathWithNFD() {
        return getPathWithNFDTmep(NFD_OpenDialog, "txt,json");
    }

    std::string getSavePathWithNFD(const std::string& default_path = {}) {
        return getPathWithNFDTmep(NFD_SaveDialog, "json", default_path.c_str());
    }
};

void AddNFDButtons(fase::ImGuiEditor& app, NFDParts& same_app) {
    app.addOptinalButton(
            "Load Pipeline", [&] { same_app.loadPipelineWithNFD(); },
            "Load a pipeline with NativeFileDialog.");
    app.addOptinalButton(
            "Save Pipeline", [&] { same_app.savePipelineWithNFD(); },
            "Save a focused pipeline with NativeFileDialog.");
}

#else // #ifdef USE_NFD

class NFDParts : public fase::PartsBase {};

void AddNFDButtons(fase::ImGuiEditor&, NFDParts&) {}

#endif // #ifdef USE_NFD

#endif // EXTRA_PARTS_H_20190329
