#include "catch.hpp"

#include "fase.h"
using namespace fase;

void Add(const int& a, const int& b, int& dst) {
    std::cout << "Add" << std::endl;
    dst = a + b;
}

void Square(const int& in, int& dst) {
    std::cout << "Square" << std::endl;
    dst = in * in;
}

void Print(const int& in) {
    std::cout << "Print : " << in << std::endl;
}

TEST_CASE("Core test") {
    FaseCore core;
    std::function<void(const int&, const int&, int&)> add = Add;
    std::function<void(const int&, int&)> square = Square;
    std::function<void(const int&)> print = Print;

    SECTION("1") {
        core.addFunctionBuilder("Add", add, {"const int&", "const int", "int&"},
                                {1, 2});
        core.addFunctionBuilder("Print", print, {"const int&"}, {1});

        core.makeNode("add1", "Add");
        core.makeNode("add2", "Add");
        core.makeNode("add3", "Add");
        core.makeNode("print1", "Print");
        core.makeNode("print2", "Print");

        core.linkNode("add1", 2, "add2", 0);
        core.linkNode("add2", 2, "add3", 0);
        core.linkNode("add3", 2, "print1", 0);

        core.setNodeArg("print2", 0, 100);

        REQUIRE(core.build());
        REQUIRE(core.run());
    }
}
