
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

constexpr char kPipelineNameKey[] = "pipeline_name";
constexpr char kPipelineKey[] = "content";

constexpr char kLinksKey[] = "Links";
constexpr char kNodesKey[] = "Nodes";
constexpr char kInputKey[] = "Inputs";
constexpr char kOutputKey[] = "Outputs";

constexpr char kLinkSrcNNameKey[] = "src_node";
constexpr char kLinkSrcArgKey[] = "src_arg";
constexpr char kLinkDstNNameKey[] = "dst_node";
constexpr char kLinkDstArgKey[] = "dst_arg";

constexpr char kNodeFuncNameKey[] = "func_name";
constexpr char kNodePriorityKey[] = "priority";
constexpr char kNodeArgsKey[] = "args";

constexpr char kNodeArgNameKey[] = "name";
constexpr char kNodeArgValueKey[] = "value";
constexpr char kNodeArgTypeKey[] = "type";

namespace {

size_t ArgNameToIdx(const string& arg_name, const FunctionUtils& f_utils) {
    for (size_t i = 0; i < f_utils.arg_names.size(); i++) {
        if (f_utils.arg_names[i] == arg_name) {
            return i;
        }
    }
    return size_t(-1);
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

const static TypeStringConverters::Deserializer dummy = [](auto&, auto&) {};

const TypeStringConverters::Deserializer&
getDeserializer(const string& type_name, const TSCMap& tsc_map) {
    for (auto& [_, tsc] : tsc_map) {
        if (tsc.name == type_name) {
            return tsc.deserializer;
        }
    }
    return dummy;
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

json11::Json getLinksJson(const PipelineAPI& pipe,
                          const map<string, FunctionUtils>& f_util_map
                                  FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        json11::Json::array link_json_array;
        auto get_args = [&](const string& n_name, size_t i) {
            return f_util_map.at(pipe.getNodes().at(n_name).func_name)
                    .arg_names[i];
        };
        for (auto& link : pipe.getLinks()) {
            link_json_array.emplace_back(json11::Json::object({
                    {kLinkSrcNNameKey, link.src_node},
                    {kLinkSrcArgKey, get_args(link.src_node, link.src_arg)},
                    {kLinkDstNNameKey, link.dst_node},
                    {kLinkDstArgKey, get_args(link.dst_node, link.dst_arg)},
            }));
        }
        return link_json_array;
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
}

json11::Json getNodeArgsJson(const Node& node,
                             const map<string, FunctionUtils>& f_util_map,
                             const TSCMap& utils FASE_COMMA_DEBUG_LOC(loc)) {
    std::vector<json11::Json::object> arg_jsons;
    try {
        for (size_t i = 0; i < node.args.size(); i++) {
            const Variable& arg = node.args[i];
            if (!utils.count(arg.getType())) {
                continue;
            }
            arg_jsons.push_back({
                    {kNodeArgNameKey,
                     f_util_map.at(node.func_name).arg_names[i]},
                    {kNodeArgValueKey, utils.at(arg.getType()).serializer(arg)},
                    {kNodeArgTypeKey, utils.at(arg.getType()).name},
            });

            // arg_json_map[f_util_map.at(node.func_name).arg_names[i]] =
            //         json11::Json::object{
            //                 {kNodeArgValueKey,
            //                  utils.at(arg.getType()).serializer(arg)},
            //                 {kNodeArgTypeKey, utils.at(arg.getType()).name},
            //         };
        }
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }

    return arg_jsons;
}

json11::Json getNodesJson(const PipelineAPI& pipe,
                          const map<string, FunctionUtils>& f_util_map,
                          const TSCMap& utils FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        json11::Json::object nodes_json_map;
        for (auto& [n_name, node] : pipe.getNodes()) {
            if (n_name == InputNodeName() || n_name == OutputNodeName()) {
                continue;
            }
            json11::Json::object node_json{
                    {kNodeFuncNameKey, node.func_name},
                    {kNodePriorityKey, node.priority},
                    {kNodeArgsKey, getNodeArgsJson(node, f_util_map, utils)},
            };
            nodes_json_map[n_name] = node_json;
        }
        return nodes_json_map;
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
}

bool LoadInOutputFromJson(const json11::Json& pipe_json, PipelineAPI& pipe_api,
                          const TSCMap& tsc_map FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        vector<string> arg_names;
        for (auto& arg_json : pipe_json[kInputKey].array_items()) {
            arg_names.emplace_back(arg_json[kNodeArgNameKey].string_value());
        }
        pipe_api.supposeInput(arg_names);
        for (size_t i = 0; i < arg_names.size(); i++) {
            auto& arg_json = pipe_json[kInputKey].array_items()[i];
            Variable v;
            strToVar(arg_json[kNodeArgValueKey].string_value(),
                     arg_json[kNodeArgTypeKey].string_value(), tsc_map, &v);
            pipe_api.setArgument(InputNodeName(), i, v);
        }
        arg_names.clear();

        for (auto& arg_json : pipe_json[kOutputKey].array_items()) {
            arg_names.emplace_back(arg_json[kNodeArgNameKey].string_value());
        }
        pipe_api.supposeOutput(arg_names);
        for (size_t i = 0; i < arg_names.size(); i++) {
            auto& arg_json = pipe_json[kOutputKey].array_items()[i];
            Variable v;
            strToVar(arg_json[kNodeArgValueKey].string_value(),
                     arg_json[kNodeArgTypeKey].string_value(), tsc_map, &v);
            pipe_api.setArgument(OutputNodeName(), i, v);
        }
        return true;
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
}

bool LoadNodeFromJson(const string& n_name, const json11::Json& node_json,
                      PipelineAPI& pipe_api, const TSCMap& tsc_map,
                      const map<string, FunctionUtils>& f_util_map
                              FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        pipe_api.newNode(n_name);
        auto f_name = node_json[kNodeFuncNameKey].string_value();
        pipe_api.allocateFunc(f_name, n_name);
        pipe_api.setPriority(n_name, node_json[kNodePriorityKey].int_value());
        for (auto& arg_json : node_json[kNodeArgsKey].array_items()) {
            std::string arg_name = arg_json[kNodeArgNameKey].string_value();

            size_t idx = ArgNameToIdx(arg_name, f_util_map.at(f_name));
            string arg_v_str = arg_json[kNodeArgTypeKey].string_value();
            auto& deserializer = getDeserializer(arg_v_str, tsc_map);
            Variable v;
            deserializer(v, arg_json[kNodeArgValueKey].string_value());
            pipe_api.setArgument(n_name, idx, v);
        }
        return true;
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
}

bool LoadPipelineFromJson(const json11::Json& pipe_json, PipelineAPI& pipe_api,
                          const TSCMap& tsc_map FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        LoadInOutputFromJson(pipe_json, pipe_api, tsc_map);

        auto f_util_map = pipe_api.getFunctionUtils();

        for (auto& [n_name, node_json] : pipe_json[kNodesKey].object_items()) {
            LoadNodeFromJson(n_name, node_json, pipe_api, tsc_map, f_util_map);
        }

        auto& link_json_array = pipe_json[kLinksKey].array_items();
        for (auto& link_json : link_json_array) {
            auto& src_n_name = link_json[kLinkSrcNNameKey].string_value();
            const Node& src_node = pipe_api.getNodes().at(src_n_name);
            auto& dst_n_name = link_json[kLinkDstNNameKey].string_value();
            const Node& dst_node = pipe_api.getNodes().at(dst_n_name);

            auto s_idx = ArgNameToIdx(link_json[kLinkSrcArgKey].string_value(),
                                      f_util_map.at(src_node.func_name));
            auto d_idx = ArgNameToIdx(link_json[kLinkDstArgKey].string_value(),
                                      f_util_map.at(dst_node.func_name));
            pipe_api.smartLink(src_n_name, s_idx, dst_n_name, d_idx);
        }
        return true;

    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
}

} // namespace

std::string PipelineToString(const string& p_name, const CoreManager& cm,
                             const TSCMap& tsc_map) {
    auto pipe_to_json = [&tsc_map](const PipelineAPI& p) -> json11::Json {
        auto f_util_map = p.getFunctionUtils();
        return json11::Json::object({
                {kLinksKey, getLinksJson(p, f_util_map)},
                {kNodesKey, getNodesJson(p, f_util_map, tsc_map)},
                {kInputKey, getNodeArgsJson(p.getNodes().at(InputNodeName()),
                                            f_util_map, tsc_map)},
                {kOutputKey, getNodeArgsJson(p.getNodes().at(OutputNodeName()),
                                             f_util_map, tsc_map)},
        });
    };

    json11::Json::array pipe_json_array;

    for (auto& p_names : cm.getDependingTree().getDependenceLayer(p_name)) {
        for (auto& sub_p_name : p_names) {
            pipe_json_array.emplace_back(json11::Json::object{
                    {kPipelineNameKey, sub_p_name},
                    {kPipelineKey, pipe_to_json(cm[sub_p_name])},
            });
        }
    }
    std::reverse(pipe_json_array.begin(), pipe_json_array.end());

    pipe_json_array.emplace_back(json11::Json::object{
            {kPipelineNameKey, p_name},
            {kPipelineKey, pipe_to_json(cm[p_name])},
    });

    return json11::Json(pipe_json_array).dump();
}

bool LoadPipelineFromJson(const string& str, CoreManager* pcm,
                          const TSCMap& tsc_map FASE_COMMA_DEBUG_LOC(loc)) {
    try {
        std::string err;
        json11::Json json = json11::Json::parse(str, err);
        auto& pipe_json_array = json.array_items();
        for (auto& pipe_json : pipe_json_array) {
            auto& pipe_api = (*pcm)[pipe_json[kPipelineNameKey].string_value()];
            LoadPipelineFromJson(pipe_json[kPipelineKey], pipe_api, tsc_map);
        }
        return true;
    } catch (std::exception& e) {
        FASE_DEBUG_LOC_LOG(loc, e.what());
        throw std::runtime_error(std::string(__func__) + " caught exception");
    }
};

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
    std::ifstream input;
    try {
        input.open(filename);

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

        if (split(filename, '.').back() == "json") {
            if (!LoadPipelineFromJson(ss.str(), pcm, tsc_map)) {
                throw std::exception();
            }
        } else if (split(filename, '.').back() == "txt") {
            if (!LoadPipelineFromString(ss.str(), pcm, tsc_map)) {
                throw std::exception();
            }
        } else {
            throw std::exception();
        }

        input.close();

        return true;
    } catch (std::exception&) {
        input.close();
        return false;
    }
}

} // namespace fase
