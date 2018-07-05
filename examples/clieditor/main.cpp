
#include "../../src/fase.h"

void Add(const int& a, const int& b, int& dst) { dst = a + b; }

void Square(const int& in, int& dst) { dst = in * in; }

void Print(const int& in) { std::cout << in << std::endl; }

int main() {
    fase::Fase<fase::CLIEditor> fase;

    FaseAddFunctionBuilder(fase, Add, (const int&, const int&, int&),
                           ("src1", "src2", "dst"));
    FaseAddFunctionBuilder(fase, Square, (const int&, int&), ("src", "dst"));
    FaseAddFunctionBuilder(fase, Print, (const int&), ("src"));

    fase.addVarGenerator(std::function<int(const std::string&)>(
                [](const std::string& s) {
            return std::atoi(s.c_str());
    }));

    fase.startEditing();

    return 0;
}
