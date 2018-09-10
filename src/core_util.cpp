
#include "core_util.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>

#define ADD_TYPE_CHECKER(map, type)                         \
    map[replace(#type, " ", "_")] = [](const Variable& v) { \
        return v.isSameType<type>();                        \
    }
#define ADD_CONV(map, type, conv_f) \
    map[replace(#type, " ", "_")] = genConverter<type>(conv_f);
#define ADD_TO_STRING(map, type)                            \
    map[replace(#type, " ", "_")] = [](const Variable& v) { \
        std::stringstream sstream;                          \
        sstream << *v.getReader<type>();                    \
        return sstream.str();                               \
    }

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

template <typename T>
std::function<void(Variable&, const std::string&)> genConverter(
        const std::function<T(const std::string&)>& conv_f) {
    return [conv_f](Variable& v, const std::string& str) {
        v.create<T>(conv_f(str));
    };
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

std::string getTypeStr(const Variable& v) {
    static std::map<std::string, std::function<bool(const Variable&)>>
            type_check_map;
    if (type_check_map.empty()) {
        ADD_TYPE_CHECKER(type_check_map, bool);
        ADD_TYPE_CHECKER(type_check_map, char);
        ADD_TYPE_CHECKER(type_check_map, unsigned char);
        ADD_TYPE_CHECKER(type_check_map, short);
        ADD_TYPE_CHECKER(type_check_map, unsigned short);
        ADD_TYPE_CHECKER(type_check_map, int);
        ADD_TYPE_CHECKER(type_check_map, unsigned int);
        ADD_TYPE_CHECKER(type_check_map, long);
        ADD_TYPE_CHECKER(type_check_map, unsigned long);
        ADD_TYPE_CHECKER(type_check_map, long long);
        ADD_TYPE_CHECKER(type_check_map, unsigned long long);
        ADD_TYPE_CHECKER(type_check_map, float);
        ADD_TYPE_CHECKER(type_check_map, double);
        ADD_TYPE_CHECKER(type_check_map, long double);

        ADD_TYPE_CHECKER(type_check_map, std::string);
    }

    for (auto& pair : type_check_map) {
        if (std::get<1>(pair)(v)) {
            return std::get<0>(pair);
        }
    }
    return "";
}

bool strToVar(const std::string& val, const std::string& type, Variable* v) {
    using S = const std::string&;
    static std::map<std::string, std::function<void(Variable&, S)>> cm;

    if (cm.empty()) {
        ADD_CONV(cm, bool, [](S s) { return std::stoi(s); });
        ADD_CONV(cm, char, [](S s) { return *s.c_str(); });
        ADD_CONV(cm, unsigned char, [](S s) { return std::stoi(s); });
        ADD_CONV(cm, short, [](S s) { return std::stoi(s); });
        ADD_CONV(cm, unsigned short, [](S s) { return std::stoi(s); });
        ADD_CONV(cm, int, [](S s) { return std::stoi(s); });
        ADD_CONV(cm, unsigned int, [](S s) { return std::stol(s); });
        ADD_CONV(cm, long, [](S s) { return std::stol(s); });
        ADD_CONV(cm, unsigned long, [](S s) { return std::stoul(s); });
        ADD_CONV(cm, long long, [](S s) { return std::stoll(s); });
        ADD_CONV(cm, unsigned long long, [](S s) { return std::stoull(s); });
        ADD_CONV(cm, float, [](S s) { return std::stof(s); });
        ADD_CONV(cm, double, [](S s) { return std::stod(s); });
        ADD_CONV(cm, long double, [](S s) { return std::stold(s); });

        ADD_CONV(cm, std::string, [](S s) { return s; });
    }

    if (!exists(cm, type)) {
        return false;
    }
    cm[type](*v, val);

    return true;
}

std::string toString(const std::string& type, const Variable& v) {
    static std::map<std::string, std::function<std::string(const Variable&)>>
            cm;

    if (cm.empty()) {
        ADD_TO_STRING(cm, bool);
        ADD_TO_STRING(cm, char);
        ADD_TO_STRING(cm, unsigned char);
        ADD_TO_STRING(cm, short);
        ADD_TO_STRING(cm, unsigned short);
        ADD_TO_STRING(cm, int);
        ADD_TO_STRING(cm, unsigned int);
        ADD_TO_STRING(cm, long);
        ADD_TO_STRING(cm, unsigned long);
        ADD_TO_STRING(cm, long long);
        ADD_TO_STRING(cm, unsigned long long);
        ADD_TO_STRING(cm, float);
        ADD_TO_STRING(cm, double);
        ADD_TO_STRING(cm, long double);

        ADD_TO_STRING(cm, std::string);
    }

    if (exists(cm, type)) {
        return cm[type](v);
    }

    return "";
}

}  // namespace

std::string CoreToString(const FaseCore& core, bool val) {
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
        if (val) {
            sstream << " ";
            for (size_t i = 0; i < std::get<1>(pair).arg_values.size(); i++) {
                const Variable& var = std::get<1>(pair).arg_values[i];
                std::string type = getTypeStr(var);
                sstream << type << " " << toString(type, var) << " ";
            }
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

bool StringToCore(const std::string& str, FaseCore* core) {
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
            for (size_t i = 0; i < core->getNodes().at(words[1]).links.size();
                 i++) {
                Variable v;
                strToVar(words[i * 2 + 3], words[i * 2 + 2], &v);
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

bool SaveFaseCore(const std::string& filename, const FaseCore& core) {
    try {
        std::ofstream output(filename);

        output << CoreToString(core, true);

        output.close();

        return true;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool LoadFaseCore(const std::string& filename, FaseCore* core) {
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

        StringToCore(ss.str(), core);

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
                        {.node_name = std::get<0>(pair), .arg_idx = i});
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
