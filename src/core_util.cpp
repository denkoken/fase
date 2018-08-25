
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
        if (*linep == "") {
            linep++;
            continue;
        }

        if (linep == std::end(lines)) {
            break;
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

}  // namespace fase
