
#ifndef EXTRA_PARTS_H_20190329
#define EXTRA_PARTS_H_20190329

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

        auto [guard, pcm] = getWriter(std::chrono::seconds(3));
        if (!pcm) {
            std::clog << "NFDParts::getWriter is timeout." << std::endl;
            return;
        }

        if (!LoadPipelineFromFile(path, pcm.get(), getConverterMap())) {
            std::cerr << "Loading pipeline from " << path << " failed."
                      << std::endl;
        }
    }

    void savePipelineWithNFD() {
        auto pipe_name = std::get<1>(getReader(std::chrono::seconds(3)))
                                 ->getFocusedPipeline();
        std::string path = getSavePathWithNFD(pipe_name);
        if (path.empty()) {
            return;
        }

        auto [guard, pcm] = getReader(std::chrono::seconds(3));
        if (!pcm) {
            std::clog << "NFDParts::getReader is timeout." << std::endl;
            return;
        }

        if (!SavePipeline(pipe_name, *pcm, path, getConverterMap())) {
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
    };

    std::string getLoadPathWithNFD() {
        return getPathWithNFDTmep(NFD_OpenDialog, "txt,json");
    }

    std::string getSavePathWithNFD(const std::string& default_path = {}) {
        return getPathWithNFDTmep(NFD_SaveDialog, "json", default_path.c_str());
    }
};

#endif // EXTRA_PARTS_H_20190329
