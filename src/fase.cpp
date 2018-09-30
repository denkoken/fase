#include "fase.h"

namespace fase {

namespace {

/*
std::string getArgsStr(const std::string& f_name, const std::string& code) {
    size_t f_name_pos = code.find(f_name);
    bool f_end = false;
    int r_blacket_c = 0;
    std::stringstream sstream;
    for (auto i = std::begin(code) + long(f_name_pos); i != std::end(code); i++) {
        char c = *i;
        if (c == '(') {
            r_blacket_c += 1;
            f_end = true;
        }
        else if (c == ')') {
            r_blacket_c -= 1;
        }
        sstream << c;

        if (f_end && !r_blacket_c) {
            return sstream.str();
        }
    }
    return "";
}

std::vector<std::string> getArgStrs(const std::string& f_name, const std::string& code) {
    std::string args_str = getArgsStr(f_name, code);

    std::vector<std::string> dst;
    std::vector<std::stringstream> buf;

    buf.push_back(std::stringstream{});

    int r_blacket_c = 0;

    for (auto i = std::begin(args_str) + long(f_name.size()) + 1; i + 1 != std::end(args_str); i++) {
        char c = *i;
        if (c == '(') {
            r_blacket_c += 1;
        }
        else if (c == ')') {
            r_blacket_c -= 1;
        }

        if (c == ',' && r_blacket_c == 0) {
            dst.push_back(buf.back().str());
            buf.push_back(std::stringstream{});
        } else {
            buf.back() << c;
        }

    }
    dst.push_back(buf.back().str());

    return dst;
}
*/

}

}
