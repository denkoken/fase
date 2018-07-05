#include "editor.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

namespace fase {

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

std::vector<std::string> clInput() {
    std::cout << "> ";
    std::string input_str;
    std::getline(std::cin, input_str);

    return split(input_str, ' ');
}

template <typename... Str>
void printError(const Str&... strs) {
    const std::vector<std::string> vec = {strs...};
    std::cout << "Error: ";
    for (size_t i = 0; i < vec.size(); i++) {
        std::cout << vec[i];
    }
    std::cout << std::endl;
}

void add(fase::FaseCore* core, const std::vector<std::string> &input) {
    if (input.size() < 3) {
        printError("Invalid arguments (<func_repr>, <node_name>)");
        return;
    }
    const std::string& func_repr = input[1];
    const std::string& node_name = input[2];

    // Find function
    const std::map<std::string, Function>& functions = core->getFunctions();
    if (!exists(functions, func_repr)) {
        printError("Invalid function repr");
        return;
    }

    // Add
    if (!core->addNode(input[2], input[1])) {
        printError("Failed");
        return;
    }
}

void del(fase::FaseCore* core, const std::vector<std::string> &input) {
    if (input.size() < 2) {
        printError("Invalid arguments (<node_name>)");
        return;
    }
    const std::string& node_name = input[1];
    core->delNode(node_name);
}

void link(fase::FaseCore* core, const std::vector<std::string> &input) {
    if (input.size() < 5) {
        printError("Invalid arguments (<src_node_name>, <src_arg_idx>, ",
                   "<dst_node_name>, <dst_arg_idx>");
        return;
    }
    const std::string& src_node_name = input[1];
    const size_t src_arg_idx = std::stoull(input[2]);
    const std::string& dst_node_name = input[3];
    const size_t dst_arg_idx = std::stoull(input[4]);

    if (!core->linkNode(src_node_name, src_arg_idx, dst_node_name,
                        dst_arg_idx)) {
        printError("Failed");
        return;
    }
}

void setArg(fase::FaseCore* core, const std::vector<std::string> &input) {
    if (input.size() < 4) {
        printError("Invalid arguments (<node_name>, <arg_idx>, <value>");
        return;
    }
    const std::string& node_name = input[1];
    const size_t arg_idx = std::stoull(input[2]);
    const std::string& value_str = input[3];

    Variable var;
    if (!core->setNodeArg(node_name, arg_idx, var)) {
        printError("Failed");
        return;
    }
}

void show(fase::FaseCore* core, const std::vector<std::string> &input) {
    std::cout << core->genNativeCode() << std::endl;
}

void run(fase::FaseCore* core, const std::vector<std::string>& input) {
    try {
        if (input.size() == 1 || input[1] != std::string("nobuild")) {
            core->build();
        }
        core->run();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

}  // anonymous namespace

void CLIEditor::start(FaseCore* core) {
    using Command = std::function<void(FaseCore*,
                                  const std::vector<std::string>&)>;

    std::map<std::string, Command> commands = {
        {"add", add},
        {"del", del},
        {"link", link},
        {"setArg", setArg},
        {"show", show},
        {"run", run},
    };
    const std::vector<std::string> exit_commands = {"quit", "exit", "logout"};

    while (true) {
        // Get command line input
        const std::vector<std::string> input = clInput();
        if (input.empty()) {
            continue;
        }

        // Check exit command
        if (exists(exit_commands, input[0])) {
            break;
        }

        // Check valid command
        if (exists(commands, input[0])) {
            commands[input[0]](core, input);
            continue;
        }

        printError("Undefined Command (", input[0], ")");
    }
}

}  // namespace fase
