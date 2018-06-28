
#include "../../src/fase.h"

void Add(const int& a, const int& b, int& dst) { dst = a + b; }

void Square(const int& in, int& dst) { dst = in * in; }

int main() {
    fase::Fase fase;
    fase.setEditor<fase::editor::CLIEditor>();

    std::array<std::string, 3> add_arg{{"a", "b", "dst"}};
    faseAddFunctionBuilder(fase, Add, add_arg, const int&, const int&, int&);

    std::array<std::string, 2> sq_arg{{"in", "dst"}};
    faseAddFunctionBuilder(fase, Square, sq_arg, const int&, int&);

    fase.startEditing();
}
