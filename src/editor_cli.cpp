#include "editor_cli.h"

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

void add(fase::FaseCore* core, CLIEditor* cli_editor,
         const std::vector<std::string>& input) {
    (void)cli_editor;
    if (input.size() < 3) {
        printError("Invalid arguments (<func_repr>, <node_name>)");
        return;
    }
    const std::string& func_repr = input[1];
    const std::string& node_name = input[2];

    // Add
    if (!core->addNode(node_name, func_repr)) {
        printError("Failed");
        return;
    }
}

void del(fase::FaseCore* core, CLIEditor* cli_editor,
         const std::vector<std::string>& input) {
    (void)cli_editor;
    if (input.size() < 2) {
        printError("Invalid arguments (<node_name>)");
        return;
    }
    const std::string& node_name = input[1];
    core->delNode(node_name);
}

void link(fase::FaseCore* core, CLIEditor* cli_editor,
          const std::vector<std::string>& input) {
    (void)cli_editor;
    if (input.size() < 5) {
        printError("Invalid arguments (<src_node_name>, <src_arg_idx>, ",
                   "<dst_node_name>, <dst_arg_idx>");
        return;
    }
    const std::string& src_node_name = input[1];
    const size_t src_arg_idx = std::stoull(input[2]);
    const std::string& dst_node_name = input[3];
    const size_t dst_arg_idx = std::stoull(input[4]);

    if (!core->addLink(src_node_name, src_arg_idx, dst_node_name,
                       dst_arg_idx)) {
        printError("Failed");
        return;
    }
}

void setArg(fase::FaseCore* core, CLIEditor* cli_editor,
            const std::vector<std::string>& input) {
    if (input.size() < 4) {
        printError("Invalid arguments (<node_name>, <arg_idx>, <value>)");
        return;
    }
    const std::string& node_name = input[1];
    const size_t arg_idx = std::stoull(input[2]);
    const std::string& value_str = input[3];

    // Find function repr
    const std::map<std::string, Node>& nodes = core->getNodes();
    if (!exists(nodes, node_name)) {
        printError("Invalid node name");
        return;
    }
    const std::string& func_repr = nodes.at(node_name).func_repr;

    // Get argument types
    const std::map<std::string, Function>& functions = core->getFunctions();
    const Function& function = functions.at(func_repr);
    const std::vector<const std::type_info*> arg_types = function.arg_types;

    // Get argument type
    if (arg_types.size() <= arg_idx) {
        printError("Invalid argument index");
        return;
    }
    const std::type_info* arg_type = arg_types[arg_idx];

    // Get variable generator
    auto generators = cli_editor->getVarGenerators();
    if (!exists(generators, arg_type)) {
        printError("Non-supported argument type");
        return;
    }
    const std::function<Variable(const std::string&)>& func =
            generators.at(arg_type);

    // Convert to a variable and set
    Variable var = func(value_str);
    if (!core->setNodeArg(node_name, arg_idx, value_str, var)) {
        printError("Failed");
        return;
    }
}

void show(fase::FaseCore* core, CLIEditor* cli_editor,
          const std::vector<std::string>& input) {
    (void)input, (void)cli_editor;
    std::cout << core->genNativeCode() << std::endl;
}

void run(fase::FaseCore* core, CLIEditor* cli_editor,
         const std::vector<std::string>& input) {
    (void)cli_editor;
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
    using Command = std::function<void(FaseCore*, CLIEditor*,
                                       const std::vector<std::string>&)>;

    const std::map<std::string, Command> commands = {
            {"add", add},       {"del", del},   {"link", link},
            {"setArg", setArg}, {"show", show}, {"run", run},
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
            commands.at(input[0])(core, this, input);
            continue;
        }

        printError("Undefined Command (", input[0], ")");
    }
}

}  // namespace fase
