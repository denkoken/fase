
#include "../../src/fase.h"

void Add(const int& a, const int& b, int& dst) { dst = a + b; }

void Square(const int& in, int& dst) { dst = in * in; }

int main() {
    fase::Fase fase;
    fase.setEditor<fase::editor::CLIEditor>();

    std::function<void(const int&, const int&, int&)> add = Add;
    fase.addFunctionBuilder("Add", std::move(add), {{"a", "b", "dst"}});
    // fase.addFunctionBuilder("Square", Square, {"in", "dst"});

    fase.startEditing();
}
