#include "catch.hpp"

#include "fase/fase.h"
using namespace fase;

static void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

static void Square(const int& in, int& dst) {
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
        REQUIRE(core.addNode("add1", "Add"));
        REQUIRE(core.addNode("square1", "Square"));
        REQUIRE(core.addLink("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 0, "10", 10));
        REQUIRE(core.setNodeArg("add1", 1, "20", 20));
        REQUIRE(core.build());
        core.run();
        REQUIRE(core.getOutput<int>("square1", 1) == 900);  // (10 + 20) ** 2
    }

    SECTION("Build with default argument") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&), 30));
        REQUIRE(FaseCoreAddFunctionBuilder(core, Square, (const int&, int&)));
        REQUIRE(core.addNode("add1", "Add"));
        REQUIRE(core.addNode("square1", "Square"));
        REQUIRE(core.addLink("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 1, "20", 20));
        REQUIRE(core.build());
        core.run();
        REQUIRE(core.getOutput<int>("square1", 1) == 2500);  // (30 + 20) ** 2
        std::cout << GenNativeCode(core, {}) << std::endl;
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
        REQUIRE(core.addNode("add1", "Add"));
        // Function `Square` is not registered
        REQUIRE_FALSE(core.addNode("square1", "Square"));
        REQUIRE_FALSE(core.addLink("add1", 2, "square1", 0));

        REQUIRE(core.setNodeArg("add1", 1, "20", 20));
        // Invalid argument index
        REQUIRE_FALSE(core.setNodeArg("add1", 3, "40", 40));
        // Invalid node name
        REQUIRE_FALSE(core.setNodeArg("square1", 0, "0", 0));

        REQUIRE(core.build());
        core.run();
        REQUIRE(core.getOutput<int>("add1", 2) == 20);
    }

    SECTION("Sub pipeline") {
        REQUIRE(FaseCoreAddFunctionBuilder(core, Add,
                                           (const int&, const int&, int&), 30));
        REQUIRE(FaseCoreAddFunctionBuilder(core, Square, (const int&, int&)));
        // make sub pipeline "test".
        REQUIRE(core.makeSubPipeline("test"));

        // write contents of sub pipeline "test".
        core.switchPipeline("test");

        REQUIRE(core.addInput("in1"));
        REQUIRE(core.addInput("in2"));
        REQUIRE(core.addOutput("dst"));

        REQUIRE(core.addNode("add", "Add"));
        REQUIRE(core.addNode("square", "Square"));

        REQUIRE(core.addLink(InputNodeStr(), 0, "add", 0));
        REQUIRE(core.addLink(InputNodeStr(), 1, "add", 1));
        REQUIRE(core.addLink("add", 2, "square", 0));
        REQUIRE(core.addLink("square", 1, OutputNodeStr(), 0));

        // use "test" function.
        core.switchPipeline("Untitled");

        REQUIRE(core.addNode("sub_p_test", SubPipelineFuncStr("test")));
        REQUIRE(core.setNodeArg("sub_p_test", 0, "1", 1));
        REQUIRE(core.setNodeArg("sub_p_test", 1, "2", 2));

        REQUIRE(core.build());
        core.run();

        REQUIRE(core.getOutput<int>("sub_p_test", 2) == 9);
    }
}
