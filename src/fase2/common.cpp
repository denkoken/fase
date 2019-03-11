
#include "common.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "constants.h"
#include "manager.h"

namespace fase {

using std::string, std::vector, std::map, std::type_index;
using size_t = std::size_t;

constexpr char SUB_PIPELINE_HEADER[] = "SUB_PIPELINE";
constexpr char MAIN_PIPELINE_HEADER[] = "MAIN_PIPELINE";
constexpr char INOUT_HEADER[] = "INOUT";
constexpr char NODE_HEADER[] = "NODE";
constexpr char LINK_HEADER[] = "LINK";
constexpr char LINK_FOOTER[] = "END_LINK";

constexpr char SPACE_REPLACED_WORD[] = "@";

namespace {

string getTypeStr(const Variable& v, const TSCMap& tsc_map) {
    if (!tsc_map.count(v.getType())) {
        return "";
    }
    return tsc_map.at(v.getType()).name;
}

bool strToVar(const std::string& val, const std::string& type,
              const TSCMap& tsc_map, Variable* v) {
    for (const auto& [_, converters] : tsc_map) {
        if (type == replace(converters.name, " ", SPACE_REPLACED_WORD)) {
            converters.deserializer(*v, val);
            return true;
        }
    }

    return false;
}

std::string toString(const Variable& v, const TSCMap& tsc_map) {
    if (!tsc_map.count(v.getType())) {
        return "";
    }
    return replace(tsc_map.at(v.getType()).serializer(v), " ",
                   SPACE_REPLACED_WORD);
}

bool PipelineToString(std::stringstream& sstream, const string& p_name,
                      const CoreManager& cm, const TSCMap& tsc_map) {
    // Inputs and Outputs
    sstream << std::string(INOUT_HEADER) << std::endl;

    const PipelineAPI& pipe_api = cm[p_name];
    const map<string, Node>& nodes = pipe_api.getNodes();

    const auto function_utils = cm.getFunctionUtils(p_name);

    const FunctionUtils& in_f = function_utils.at(kInputFuncName);
    for (const std::string& name : in_f.arg_names) {
        sstream << " " << name;
    }
    sstream << std::endl;

    const FunctionUtils& out_f = function_utils.at(kOutputFuncName);
    for (const std::string& name : out_f.arg_names) {
        sstream << " " << name;
    }
    sstream << std::endl;

    // Nodes
    sstream << std::string(NODE_HEADER) << std::endl;

    for (const auto& [n_name, node] : nodes) {
        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            continue;
        }

        sstream << node.func_name << " " << n_name << " ";
        for (size_t i = 0; i < node.args.size(); i++) {
            const Variable& var = node.args[i];
            if (tsc_map.count(var.getType())) {
                sstream << getTypeStr(var, tsc_map) << " "
                        << toString(var, tsc_map) << " ";
            } else {
                sstream << "  ";
            }
        }

        sstream << std::endl;
    }

    // Links
    sstream << std::string(LINK_HEADER) << std::endl;

    for (auto& [s_n_name, s_idx, d_n_name, d_idx] : pipe_api.getLinks()) {
        sstream << d_n_name << " " << d_idx << " " << s_n_name << " " << s_idx
                << std::endl;
    }

    sstream << std::string(LINK_FOOTER) << std::endl;

    // TODO

    return true;
}

template <class LineIterator>
std::string Next(LineIterator& linep) {
    while (*linep == "") {
        linep++;
    }
    return *linep;
}

template <class LineIterator>
bool StringToPipeline(const string& p_name, LineIterator& linep,
                      CoreManager* pcm, const TSCMap& tsc_map) {
    if (Next(linep) != std::string(INOUT_HEADER)) {
        std::cerr << "invalid file layout" << std::endl;
        return false;
    }

    linep++;

    const auto func_utils = pcm->getFunctionUtils(p_name);
    auto& pipe_api = (*pcm)[p_name];

    {
        auto words = split(Next(linep), ' ');
        erase_all(words, "");
        if (!pipe_api.supposeInput(words)) {
            std::cerr << "supposeInput failed : line\"" << *linep << "\""
                      << std::endl;
        }
        linep++;
    }
    {
        auto words = split(Next(linep), ' ');
        erase_all(words, "");
        if (!pipe_api.supposeOutput(words)) {
            std::cerr << "supposeOutput failed : line\"" << *linep << "\""
                      << std::endl;
        }
        linep++;
    }

    assert(Next(linep) == NODE_HEADER);
    linep++;

    // Nodes
    while (true) {
        if (Next(linep) == std::string(LINK_HEADER)) {
            linep++;
            break;
        }
        const auto words = split(Next(linep), ' ');
        if (!pipe_api.newNode(words.at(1))) {
            std::cerr << "addNode failed" << std::endl;
            return false;
        }
        if (!pipe_api.allocateFunc(words.at(0), words.at(1))) {
            std::cerr << "allocateFunc failed" << std::endl;
            return false;
        }

        if (words.size() > 2) {
            const size_t& arg_n = pipe_api.getNodes().at(words[1]).args.size();
            for (size_t i = 0; i < arg_n; i++) {
                if (words[i * 2 + 3].empty()) {
                    continue;
                }
                Variable v;
                strToVar(words[i * 2 + 3], words[i * 2 + 2], tsc_map, &v);
                pipe_api.setArgument(words[1], i, v);
            }
        }
        linep++;
    }

    // Links
    while (true) {
        if (Next(linep) == std::string(LINK_FOOTER)) {
            break;
        }

        auto words = split(Next(linep), ' ');
        if (!pipe_api.smartLink(words.at(2), std::stoul(words.at(3)),
                                words.at(0), std::stoul(words.at(1)))) {
            std::cerr << "smartLink failed : line\"" << *linep << "\""
                      << std::endl;
            return false;
        }
        linep++;
    }
    return true;
}

}  // namespace

string GenNativeCode(const string& p_name, const CoreManager& cm,
                     const TSCMap& tsc_map, const string& entry_name,
                     const string& indent) {}

string PipelineToString(const string& p_name, const CoreManager& cm,
                        const TSCMap& tsc_map) {
    std::stringstream sstream;

    auto sub_pipes = cm.getDependingTree().getDependenceLayer(p_name);
    for (auto iter = sub_pipes.rbegin(); iter != sub_pipes.rend(); iter++) {
        for (auto& sub_pipe_name : *iter) {
            sstream << SUB_PIPELINE_HEADER << std::endl;
            sstream << sub_pipe_name << std::endl;
            PipelineToString(sstream, sub_pipe_name, cm, tsc_map);
            sstream << std::endl;
        }
    }

    sstream << MAIN_PIPELINE_HEADER << std::endl;
    sstream << p_name << std::endl;
    PipelineToString(sstream, p_name, cm, tsc_map);

    return sstream.str();
}

bool LoadPipelineFromString(const string& str, CoreManager* pcm,
                            const TSCMap& tsc_map) {
    std::vector<std::string> lines = split(str, '\n');
    auto& cm = *pcm;

    auto linep = std::begin(lines);

    while (true) {
        if (Next(linep) == SUB_PIPELINE_HEADER ||
            Next(linep) == MAIN_PIPELINE_HEADER) {
            linep++;
            string p_name = Next(linep);
            if (exists(p_name, cm.getPipelineNames())) {
                std::cerr << "LoadPipelineFromString : "
                             "a same name pipeline allready exists."
                          << std::endl;
                return false;
            }
            cm[p_name];
            linep++;
            if (!StringToPipeline(p_name, linep, pcm, tsc_map)) {
                return false;
            }
        }
        if (++linep == std::end(lines)) {
            break;
        }
        bool f = false;
        while (*linep == "") {
            if (++linep == std::end(lines)) {
                f = true;
            }
        }
        if (f) {
            break;
        }
    }
    return true;
}

bool SavePipeline(const std::string& p_name, const CoreManager& cm,
                  const std::string& filename, const TSCMap& tsc_map) {
    try {
        std::ofstream output(filename);

        output << PipelineToString(p_name, cm, tsc_map);

        output.close();

        return true;
    } catch (std::exception& e) {
        std::cerr << "SavePipeline Error : " << e.what() << std::endl;
        return false;
    }
}

bool LoadPipelineFromFile(const string& filename, CoreManager* pcm,
                          const TSCMap& tsc_map) {
    try {
        std::ifstream input(filename);

        if (!input) {
            std::cerr << "file opening is failed : " << filename;
            return false;
        }

        std::stringstream ss;

        while (!input.eof()) {
            std::string buf;
            std::getline(input, buf);
            ss << buf << std::endl;
        }

        if (!LoadPipelineFromString(ss.str(), pcm, tsc_map)) {
            throw std::exception();
        }

        input.close();

        return true;
    } catch (std::exception&) {
        return false;
    }
}

}  // namespace fase
