
#include "editor.h"

#include <iostream>
#include <sstream>

namespace {

std::vector<std::string> split(const std::string& str, const char& sep) {
    std::vector<std::string> dst;
    std::stringstream ss{str};
    std::string buf;
    while (std::getline(ss, buf, sep)) {
        dst.push_back(buf);
    }
    return dst;
}

}  // namespace

namespace fase {

namespace editor {

void CLIEditor::start(pe::FaseCore* core, Variable variable) {
    while (true) {
        std::cout << " > ";
        std::string input_str;
        std::getline(std::cin, input_str);
        std::cout << input_str << std::endl;

        std::vector<std::string> input = split(input_str, ' ');

        if (input[0] == std::string("quit")) {
            break;
        }
    }
}

}  // namespace editor

}  // namespace fase
