
#include "core_util.h"

#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace {

constexpr char NODE_HEADER[] = "NODE";
constexpr char LINK_HEADER[] = "LINK";

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

}  // namespace

namespace fase {

std::string CoreToString(const FaseCore& core) {
    std::stringstream sstream;

    sstream << std::string(NODE_HEADER) << std::endl;

    for (const auto& pair : core.getNodes()) {
        sstream << std::get<1>(pair).func_repr << " " << std::get<0>(pair)
                << std::endl;
    }

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

    if (*linep != std::string(NODE_HEADER)) {
        return false;
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
        auto words = split(*linep, ' ');
        if (!core->addNode(words.at(1), words.at(0))) {
            return false;
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

        output << CoreToString(core);

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
    std::vector<std::set<std::string>> dst;

    std::set<std::string> unused_node_names;

    for (const auto& pair : nodes) {
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
