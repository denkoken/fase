#include "catch.hpp"

#include "fase.h"
using namespace fase;

void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

#define FaseCoreAddFunctionBuilder(core, func, arg_types, ...) \
    FaseAddFunctionBuilder(core, func, arg_types, (""), __VA_ARGS__)

TEST_CASE("Core test") {
    FaseCore core;

    SECTION("Build basic") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&)));
        REQUIRE(FaseCoreAddFunctionBuilder(core, Square, (const int&, int&)));
        REQUIRE(core.makeNode("add1", "Add"));
        REQUIRE(core.makeNode("square1", "Square"));
        REQUIRE(core.linkNode("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 0, 10));
        REQUIRE(core.setNodeArg("add1", 1, 20));
        REQUIRE(core.build());
        REQUIRE(core.run());
        REQUIRE(core.getOutput<int>("square1", 1) == 900);  // (10 + 20) ** 2
    }

    SECTION("Build with default argument") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&), 30));
        REQUIRE(FaseCoreAddFunctionBuilder(core, Square, (const int&, int&)));
        REQUIRE(core.makeNode("add1", "Add"));
        REQUIRE(core.makeNode("square1", "Square"));
        REQUIRE(core.linkNode("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 1, 20));
        REQUIRE(core.build());
        REQUIRE(core.run());
        REQUIRE(core.getOutput<int>("square1", 1) == 2500);  // (30 + 20) ** 2
        std::cout << core.genNativeCode() << std::endl;
    }

    SECTION("Duplicate AddFunctionBuilder") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&)));
        REQUIRE_FALSE(FaseCoreAddFunctionBuilder(
                core, Add, (const int&, const int&, int&)));
    }

    SECTION("Detect invalid node") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&)));
        REQUIRE(core.makeNode("add1", "Add"));
        // Function `Square` is not registered
        REQUIRE_FALSE(core.makeNode("square1", "Square"));
        REQUIRE_FALSE(core.linkNode("add1", 2, "square1", 0));

        REQUIRE(core.setNodeArg("add1", 1, 20));
        // Invalid argument index
        REQUIRE_FALSE(core.setNodeArg("add1", 3, 40));
        // Invalid node name
        REQUIRE_FALSE(core.setNodeArg("square1", 0, 0));

        REQUIRE(core.build());
        REQUIRE(core.run());
        REQUIRE(core.getOutput<int>("add1", 2) == 20);
    }
}
