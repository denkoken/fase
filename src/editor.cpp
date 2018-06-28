
#include "editor.h"

#include <iostream>
#include <sstream>
#include <map>

namespace {

template <typename T, typename S>
inline bool exists(const std::map<T, S>& map, const T& key) {
    return map.find(key) != std::end(map);
}

template <class T, typename S>
inline bool exists(const T& list, const S& key) {
    return std::find(std::begin(list), std::end(list), key) != std::end(list);
}

std::vector<std::string> split(const std::string& str, const char& sep) {
    std::vector<std::string> dst;
    std::stringstream ss{str};
    std::string buf;
    while (std::getline(ss, buf, sep)) {
        dst.push_back(buf);
    }
    return dst;
}

std::vector<std::string> clInput() {
    std::cout << " > ";
    std::string input_str;
    std::getline(std::cin, input_str);

    return split(input_str, ' ');
}

void add(fase::pe::FaseCore* core, std::vector<std::string> input) {
    std::cout << "add" << std::endl;
}

}  // namespace

namespace fase {

namespace editor {

void CLIEditor::start(pe::FaseCore* core, Variable variable) {
    using std::string;
    using Command = std::function<void(pe::FaseCore*, std::vector<string>)>;

    const string exit_commands[] = {"quit", "exit", "logout"};

    std::map<string, Command> commands;

    commands["add"] = add;

    while (true) {
        std::vector<string> input = clInput();

        if (exists(exit_commands, input[0])) {
            break;
        }

        if (!exists(commands, input[0])) {
            std::cout << "Undefined Command : "<< input[0] << std::endl;
            continue;
        }

        commands[input[0]](core, input);
    }
}

}  // namespace editor

}  // namespace fase
