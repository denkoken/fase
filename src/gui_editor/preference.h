#ifndef GUI_PREFERENCE_H_20180923
#define GUI_PREFERENCE_H_20180923

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fase {
namespace guieditor {

struct GUIPreference {
    bool auto_layout = false;
    int priority_min = -10;
    int priority_max = 10;
    bool is_simple_node_box = false;
    int max_arg_name_chars = 16;
    bool enable_edit_panel = false;
    bool enable_node_list_panel = true;
    bool another_th_run = true;

    int node_list_panel_size = 150;
    int edit_panel_size = 200;
};

class GUIPreferenceManager {
public:
    GUIPreferenceManager() {
        load();
    }
    ~GUIPreferenceManager() {
        save();
    }

    GUIPreference& get() {
        return data;
    }

private:
    const std::string filename = "fase_gui.ini";
    GUIPreference data;

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

    void set(const std::vector<std::string>& line) {
        if ("auto_layout" == line[0]) {
            data.auto_layout = std::stoi(line[1]);
        } else if ("priority_min" == line[0]) {
            data.priority_min = std::stoi(line[1]);
        } else if ("priority_max" == line[0]) {
            data.priority_max = std::stoi(line[1]);
        } else if ("is_simple_node_box" == line[0]) {
            data.is_simple_node_box = std::stoi(line[1]);
        } else if ("max_arg_name_chars" == line[0]) {
            data.max_arg_name_chars = std::stoi(line[1]);
        } else if ("enable_edit_panel" == line[0]) {
            data.enable_edit_panel = std::stoi(line[1]);
        } else if ("enable_node_list_panel" == line[0]) {
            data.enable_node_list_panel = std::stoi(line[1]);
        } else if ("another_th_run" == line[0]) {
            data.another_th_run = std::stoi(line[1]);
        } else if ("node_list_panel_size" == line[0]) {
            data.node_list_panel_size = std::stoi(line[1]);
        } else if ("edit_panel_size" == line[0]) {
            data.edit_panel_size = std::stoi(line[1]);
        }
    }

    std::string toString() {
        std::stringstream ss;

        ss << "auto_layout: " << data.auto_layout << std::endl;
        ss << "priority_min: " << data.priority_min << std::endl;
        ss << "priority_max: " << data.priority_max << std::endl;
        ss << "is_simple_node_box: " << data.is_simple_node_box << std::endl;
        ss << "max_arg_name_chars: " << data.max_arg_name_chars << std::endl;
        ss << "enable_edit_panel: " << data.enable_edit_panel << std::endl;
        ss << "enable_node_list_panel: " << data.enable_node_list_panel
           << std::endl;
        ss << "another_th_run: " << data.another_th_run << std::endl;
        ss << "node_list_panel_size: " << data.node_list_panel_size
           << std::endl;
        ss << "edit_panel_size: " << data.edit_panel_size << std::endl;
        // copy set() and vi command type
        // :s/else if ("\(.*\)" == line\[0\]) {/ss << "\1: " << data.\1 <<
        // std::endl;

        return ss.str();
    }

    void load() {
        try {
            std::ifstream input((std::string(filename)));

            if (!bool(input)) {
                return;
            }

            while (!input.eof()) {
                std::string line;
                std::getline(input, line);
                set(split(line, ':'));
            }

            input.close();
        } catch (std::exception& e) {
        }
    }

    void save() {
        try {
            std::ofstream output(filename);
            output << toString();
            output.close();
        } catch (std::exception& e) {
            std::cerr << "can't save preference : " << e.what() << std::endl;
        }
    }
};

}  // namespace guieditor
}  // namespace fase

#endif  // GUI_PREFERENCE_H_20180923
