#include "editor.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>

namespace {

template <typename T, typename S>
inline bool exists_map(const std::map<T, S>& map, const T& key) {
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
    std::cout << "> ";
    std::string input_str;
    std::getline(std::cin, input_str);

    return split(input_str, ' ');
}

void add(fase::FaseCore* core, std::vector<std::string> input) {
    if (input[1] == std::string("f")) {
        core->makeNode(input[3], input[2]);
        return;
    } else if (input[1] != std::string("v") && input[1] != std::string("c")) {
        std::cout << "Undefined Argument : " << input[1] << std::endl;
        return;
    }

    bool is_c = (input[1] == std::string("c"));
    if (input[2] == std::string("int")) {
        int val = 0;
        if (input.size() >= 5) {
            val = std::stoi(input[4]);
        }
        core->setArgument(input[3], is_c, val);
    } else if (input[2] == std::string("float")) {
        float val = 0.f;
        if (input.size() >= 5) {
            val = float(std::stod(input[4]));
        }
        core->setArgument(input[3], is_c, val);
    } else if (input[2] == std::string("double")) {
        double val = 0.f;
        if (input.size() >= 5) {
            val = std::stod(input[4]);
        }
        core->setArgument(input[3], is_c, val);
    } else {
        std::cout << "Undefined Type : " << input[2] << std::endl;
    }
}

void link(fase::FaseCore* core, std::vector<std::string> input) {
    core->linkNode(input[1], std::stoull(input[2]), input[3],
                   std::stoull(input[4]));
}

void del(fase::FaseCore* core, std::vector<std::string> input) {
    core->delNode(input[1]);
    core->delArgument(input[1]);
}

void showState(fase::FaseCore* core) {
    const auto& arguments = core->getArguments();
    const auto& nodes = core->getNodes();

    std::cout << "Arguments : " << std::endl;
    for (const auto& node : arguments) {
        if (node.second.constant) {
            std::cout << "  " << node.first << " (Constant)" << std::endl;
        } else {
            std::cout << "  " << node.first << std::endl;
        }
    }

    std::cout << "Nodes : " << std::endl;
    for (const auto& node : nodes) {
        std::cout << "  " << node.first << " (" << node.second.func_name << ")"
                  << std::endl;
    }
}

void showFunctions(fase::FaseCore* core) {
    const auto& func_infos = core->getFunctions();

    for (const auto& func : func_infos) {
        std::cout << func.first << ":" << std::endl;
        const std::vector<std::string>& arg_types =
            func.second.builder->getArgTypes();

        for (size_t i = 0; i < func.second.arg_names.size(); i++) {
            std::cout << "  " << i << " : " << func.second.arg_names[i] << " ("
                      << arg_types[i] << ")" << std::endl;
        }
    }
}

void showLinks(fase::FaseCore* core) {
    const auto& nodes = core->getNodes();

    for (const auto& node : nodes) {
        std::cout << node.first << ":" << std::endl;
        int i = 0;
        for (const auto& link : node.second.links) {
            std::cout << "  " << i++ << " : ";
            if (link.node_name == std::string("")) {
                std::cout << "Unset" << std::endl;
            } else {
                std::cout << link.node_name << " : " << link.arg_idx
                          << std::endl;
            }
        }
    }
}

void show(fase::FaseCore* core, std::vector<std::string> input) {
    if (input[1] == std::string("state") or input[1] == std::string("s")) {
        showState(core);
    } else if (input[1] == std::string("function") or
               input[1] == std::string("f")) {
        showFunctions(core);
    } else if (input[1] == std::string("link") or
               input[1] == std::string("l")) {
        showLinks(core);
    }
}

void run(fase::FaseCore* core, std::vector<std::string> input) {
    try {
        if (input.size() == 1 || input[1] != std::string("nobuild")) {
            core->build();
        }
        core->run();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

}  // namespace

namespace fase {

namespace editor {

void CLIEditor::start(FaseCore* core) {
    core->addVariableBuilder<int>();
    core->addVariableBuilder<float>();
    core->addVariableBuilder<double>();
    core->addVariableBuilder<std::string>();

    using std::string;
    using Command = std::function<void(FaseCore*, std::vector<string>)>;

    const string exit_commands[] = {"quit", "exit", "logout"};

    std::map<string, Command> commands;

    commands["add"] = add;
    commands["link"] = link;
    commands["del"] = del;
    commands["show"] = show;
    commands["run"] = run;

    while (true) {
        std::vector<string> input = clInput();

        if (input.empty()) continue;

        if (exists(exit_commands, input[0])) {
            break;
        }

        if (!exists_map(commands, input[0])) {
            std::cout << "Undefined Command : " << input[0] << std::endl;
            continue;
        }

        commands[input[0]](core, input);
    }
}

}  // namespace editor

}  // namespace fase
