
#include "common.h"

#include <json11.hpp>

#include <cassert>
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

constexpr char kLinksKey[] = "Links";
constexpr char kNodesKey[] = "Nodes";

constexpr char kLinkSrcNNameKey[] = "src_node";
constexpr char kLinkSrcArgKey[] = "src_arg";
constexpr char kLinkDstNNameKey[] = "dst_node";
constexpr char kLinkDstArgKey[] = "dst_arg";

constexpr char kNodeFuncNameKey[] = "func_name";
constexpr char kNodePriorityKey[] = "priority";
constexpr char kNodeArgsKey[] = "args";

constexpr char kNodeArgValueKey[] = "value";
constexpr char kNodeArgTypeKey[] = "type";

namespace {

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

json11::Json getLinksJson(const PipelineAPI& pipe) {
    json11::Json::array link_json_array;
    for (auto& link : pipe.getLinks()) {
        link_json_array.emplace_back(json11::Json::object({
                {kLinkSrcNNameKey, link.src_node},
                {kLinkSrcArgKey, int(link.src_arg)},
                {kLinkDstNNameKey, link.dst_node},
                {kLinkDstArgKey, int(link.dst_arg)},
        }));
    }
    return link_json_array;
}

json11::Json getNodeArgsJson(const Node& node, const TSCMap& utils) {
    json11::Json::array arg_json_array;
    for (auto& arg : node.args) {
        arg_json_array.emplace_back(json11::Json::object{
                {kNodeArgValueKey, utils.at(arg.getType()).serializer(arg)},
                {kNodeArgTypeKey, utils.at(arg.getType()).name},
        });
    }

    return arg_json_array;
}

json11::Json getNodesJson(const PipelineAPI& pipe, const TSCMap& utils) {
    json11::Json::object nodes_json_map;
    for (auto& [n_name, node] : pipe.getNodes()) {
        if (n_name == InputNodeName() || n_name == OutputNodeName()) {
            continue;
        }
        json11::Json::object node_json{
                {kNodeFuncNameKey, node.func_name},
                {kNodePriorityKey, node.priority},
                {kNodeArgsKey, getNodeArgsJson(node, utils)},
        };
        nodes_json_map[n_name] = node_json;
    }
    return nodes_json_map;
}

}  // namespace

std::string PipelineToString(const string& p_name, const CoreManager& cm,
                             const TSCMap& tsc_map) {
    json11::Json json = json11::Json::object({
            {kLinksKey, getLinksJson(cm[p_name])},
            {kNodesKey, getNodesJson(cm[p_name], tsc_map)},
    });

    return json.dump();
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
