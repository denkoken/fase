
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
    if (input[1] == std::string("f")) {
        core->makeFunctionNode(input[3], input[2]);
        return;
    }
    else if (input[1] != std::string("v") && input[1] != std::string("c")) {
        std::cout << "Undefined Argument : " << input[1] << std::endl;
        return;
    }

    bool is_c = (input[1] == std::string("c"));
    if (input[2] == std::string("int")) {
        int val = 0;
        if (input.size() >= 5) {
            val = std::stoi(input[4]);
        }
        core->makeVariableNode(input[3], is_c, val);
    }
    else if (input[2] == std::string("float")) {
        float val = 0.f;
        if (input.size() >= 5) {
            val = float(std::stod(input[4]));
        }
        core->makeVariableNode(input[3], is_c, val);
    }
    else if (input[2] == std::string("double")) {
        double val = 0.f;
        if (input.size() >= 5) {
            val = std::stod(input[4]);
        }
        core->makeVariableNode(input[3], is_c, val);
    }
}

void link(fase::pe::FaseCore* core, std::vector<std::string> input) {
    core->linkNode(input[1], std::stoull(input[2]),
                   input[3], std::stoull(input[4]));
}

void del(fase::pe::FaseCore* core, std::vector<std::string> input) {
    std::cout << "del" << std::endl;
}

void showNodes(fase::pe::FaseCore* core) {
    const auto& vnodes = core->getVariableNodes();
    const auto& fnodes = core->getFunctionNodes();

    for (const auto& node : vnodes) {
        if (node.second.constant) {
            std::cout << "c: " << node.first << std::endl;
        }
        else {
            std::cout << "v: " << node.first << std::endl;
        }
    }

    for (const auto& node : fnodes) {
        std::cout << "f: " << node.first << " ("
            << node.second.type << ")" << std::endl;
    }
}

void showFunctions(fase::pe::FaseCore* core) {
    // TODO
}

void showLinks(fase::pe::FaseCore* core) {
    // TODO
}

void show(fase::pe::FaseCore* core, std::vector<std::string> input) {
    if (input[1] == std::string("node") or input[1] == std::string("n")) {
        showNodes(core);
    }
    else if (input[1] == std::string("function") or
             input[1] == std::string("f")) {
        showFunctions(core);
    }
    else if (input[1] == std::string("link") or input[1] == std::string("l")) {
        showLinks(core);
    }
}

void run(fase::pe::FaseCore* core, std::vector<std::string> input) {
    std::cout << "run" << std::endl;
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
    commands["link"] = link;
    commands["del"] = del;
    commands["show"] = show;
    commands["run"] = run;

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
