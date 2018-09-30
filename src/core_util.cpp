
#include "core_util.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace fase {

namespace {

constexpr char INOUT_HEADER[] = "INOUT";
constexpr char NODE_HEADER[] = "NODE";
constexpr char LINK_HEADER[] = "LINK";

std::string replace(std::string str, const std::string& fr,
                    const std::string& to) {
    const size_t len = fr.length();
    for (size_t p = str.find(fr); p != std::string::npos; p = str.find(fr)) {
        str = str.replace(p, len, to);
    }

    return str;
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

std::vector<std::string> FindRunnableNodes(
        const std::set<std::string>& unused_node_names,
        const std::map<std::string, fase::Node>& nodes) {
    std::vector<std::string> dst;

    // Find runnable function node
    for (auto& node_name : unused_node_names) {
        const fase::Node& node = nodes.at(node_name);

        bool runnable = true;
        size_t arg_idx = 0;
        for (auto& link : node.links) {
            if (link.node_name.empty()) {
                // Case 1: Create default argument
            } else {
                // Case 2: Use output variable
                // Is linked node created?
                if (fase::exists(unused_node_names, link.node_name)) {
                    runnable = false;
                    break;
                }
            }
            arg_idx++;
        }

        if (runnable) {
            dst.emplace_back(node_name);
        }
    }

    return dst;
}

std::string getTypeStr(const Variable& v, const TypeUtils& utils) {
    for (auto& pair : utils.checkers) {
        if (std::get<1>(pair)(v)) {
            return replace(std::get<0>(pair), " ", "_");
        }
    }
    return "";
}

bool strToVar(const std::string& val, const std::string& type,
              const TypeUtils& utils, Variable* v) {
    for (const auto& pair : utils.deserializers) {
        if (type == replace(std::get<0>(pair), " ", "_")) {
            std::get<1>(pair)(*v, val);
            return true;
        }
    }

    return false;
}

std::string toString(const std::string& type, const Variable& v,
                     const TypeUtils& utils) {
    if (exists(utils.serializers, type)) {
        return replace(utils.serializers.at(type)(v), " ", "_");
    }

    return "";
}

}  // namespace

std::string CoreToString(const FaseCore& core, const TypeUtils& utils) {
    std::stringstream sstream;

    // Inputs and Outputs
    sstream << std::string(INOUT_HEADER) << std::endl;

    const Function& in_f = core.getFunctions().at(InputFuncStr());
    for (const std::string& name : in_f.arg_names) {
        sstream << " " << name;
    }
    sstream << std::endl;

    const Function& out_f = core.getFunctions().at(OutputFuncStr());
    for (const std::string& name : out_f.arg_names) {
        sstream << " " << name;
    }
    sstream << std::endl;

    // Nodes
    sstream << std::string(NODE_HEADER) << std::endl;

    for (const auto& pair : core.getNodes()) {
        if (std::get<0>(pair) == InputNodeStr() ||
            std::get<0>(pair) == OutputNodeStr()) {
            continue;
        }

        sstream << std::get<1>(pair).func_repr << " " << std::get<0>(pair);
        sstream << " ";
        for (size_t i = 0; i < std::get<1>(pair).arg_values.size(); i++) {
            const Variable& var = std::get<1>(pair).arg_values[i];
            std::string type = getTypeStr(var, utils);
            sstream << type << " " << toString(type, var, utils) << " ";
        }

        sstream << std::endl;
    }

    // Links
    sstream << std::string(LINK_HEADER) << std::endl;

    for (const auto& pair : core.getNodes()) {
        const Node& node = std::get<1>(pair);
        for (size_t i = 0; i < node.links.size(); i++) {
            if (node.links[i].node_name.empty()) {
                continue;
            }
            sstream << std::get<0>(pair) << " " << i << " "
                    << node.links[i].node_name << " " << node.links[i].arg_idx
                    << std::endl;
        }
    }

    // TODO

    return sstream.str();
}

bool StringToCore(const std::string& str, FaseCore* core,
                  const TypeUtils& utils) {
    std::vector<std::string> lines = split(str, '\n');

    auto linep = std::begin(lines);

    if (*linep != std::string(INOUT_HEADER)) {
        return false;
    }

    linep++;

    {
        auto words = split(*linep, ' ');
        for (auto& word : words) {
            if (!word.empty()) {
                core->addInput(word);
            }
        }
        linep++;
    }
    {
        auto words = split(*linep, ' ');
        for (auto& word : words) {
            if (!word.empty()) {
                core->addOutput(word);
            }
        }
        linep++;
    }

    linep++;

    // Nodes
    while (true) {
        if (*linep == "") {
            linep++;
            continue;
        }

        if (*linep == std::string(LINK_HEADER)) {
            linep++;
            break;
        }
        const auto words = split(*linep, ' ');
        if (!core->addNode(words.at(1), words.at(0))) {
            return false;
        }

        if (words.size() > 2) {
            const size_t& arg_n = core->getNodes().at(words[1]).links.size();
            for (size_t i = 0; i < arg_n; i++) {
                if (words[i * 2 + 3].empty()) {
                    continue;
                }
                Variable v;
                strToVar(words[i * 2 + 3], words[i * 2 + 2], utils, &v);
                core->setNodeArg(words[1], i, words[i * 2 + 3], v);
            }
        }
        linep++;
    }

    // Links
    while (true) {
        if (linep == std::end(lines)) {
            break;
        }

        if (*linep == "") {
            linep++;
            continue;
        }

        auto words = split(*linep, ' ');
        if (!core->addLink(words.at(2), std::stoul(words.at(3)), words.at(0),
                           std::stoul(words.at(1)))) {
            return false;
        }
        linep++;
    }

    // TODO

    return true;
}

bool SaveFaseCore(const std::string& filename, const FaseCore& core,
                  const TypeUtils& utils) {
    try {
        std::ofstream output(filename);

        output << CoreToString(core, utils);

        output.close();

        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool LoadFaseCore(const std::string& filename, FaseCore* core,
                  const TypeUtils& utils) {
    try {
        std::ifstream input(filename);

        if (!input) {
            return false;
        }

        std::stringstream ss;

        while (!input.eof()) {
            std::string buf;
            std::getline(input, buf);
            ss << buf << std::endl;
        }

        core->switchProject(split(filename, '.')[0]);
        StringToCore(ss.str(), core, utils);

        input.close();

        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

std::vector<std::vector<Link>> getReverseLinks(
        const std::string& node, const std::map<std::string, Node>& nodes) {
    std::vector<std::vector<Link>> dst(nodes.at(node).links.size());
    for (const auto& pair : nodes) {
        for (size_t i = 0; i < std::get<1>(pair).links.size(); i++) {
            if (std::get<1>(pair).links[i].node_name == node) {
                dst[std::get<1>(pair).links[i].arg_idx].push_back(
                        {std::get<0>(pair), i});
            }
        }
    }
    return dst;
}

std::vector<std::set<std::string>> GetCallOrder(
        const std::map<std::string, Node>& nodes) {
    std::vector<std::set<std::string>> dst = {{InputNodeStr()}};

    std::set<std::string> unused_node_names;

    for (const auto& pair : nodes) {
        if (std::get<0>(pair) == InputNodeStr()) {
            continue;
        }
        unused_node_names.insert(std::get<0>(pair));
    }

    while (true) {
        auto runnables = FindRunnableNodes(unused_node_names, nodes);

        if (runnables.empty()) break;

        std::set<int> prioritys;

        for (auto& name : runnables) {
            prioritys.insert(nodes.at(name).priority);
        }

        // get the smallest priority num
        int smallest = *std::begin(prioritys);

        std::set<std::string> buf;
        for (auto& name : runnables) {
            if (nodes.at(name).priority == smallest) {
                buf.insert(name);
                unused_node_names.erase(name);
            }
        }
        dst.emplace_back(std::move(buf));
    }

    if (!unused_node_names.empty()) {
        return std::vector<std::set<std::string>>();
    }

    return dst;
}

}  // namespace fase
