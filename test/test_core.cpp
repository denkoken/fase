#include "catch.hpp"

#include "fase2/fase.h"

#include "fase2/constants.h"

using namespace fase;

static void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

static void Square(const int& in, int& dst) {
    dst = in * in;
}

TEST_CASE("Core test") {
    Core core;
    {
        auto univ_add =
                UnivFuncGenerator<void(const int&, const int&, int&)>::Gen(
                        [&]() -> std::function<void(const int&, const int&,
                                                    int&)> { return Add; });

        std::vector<Variable> default_args(3);

        default_args[0].create<int>(1);
        default_args[1].create<int>(2);
        default_args[2].create<int>(0);

        REQUIRE(core.addUnivFunc(univ_add, "add", std::move(default_args)));
    }

    {
        auto univ_sq = UnivFuncGenerator<void(const int&, int&)>::Gen(
                [&]() -> std::function<void(const int&, int&)> {
                    return Square;
                });

        std::vector<Variable> default_args(2);

        default_args[0].create<int>(4);
        default_args[1].create<int>(0);

        REQUIRE(core.addUnivFunc(univ_sq, "square", std::move(default_args)));
    }

    REQUIRE(core.newNode("a"));
    REQUIRE(core.newNode("b"));
    REQUIRE_FALSE(core.newNode("a"));
    REQUIRE(core.allocateFunc("add", "a"));
    REQUIRE(core.allocateFunc("add", "b"));
    REQUIRE(core.allocateFunc("square", "b"));

    REQUIRE(core.run());
    REQUIRE(*core.getNodes().at("a").args[2].getReader<int>() == 3);
    REQUIRE(*core.getNodes().at("b").args[1].getReader<int>() == 16);

    REQUIRE(core.linkNode("a", 2, "b", 0));

    REQUIRE(core.run());
    REQUIRE(*core.getNodes().at("a").args[2].getReader<int>() == 3);
    REQUIRE(core.getNodes().at("b").args[0].getReader<int>() ==
            core.getNodes().at("a").args[2].getReader<int>());
    REQUIRE(*core.getNodes().at("b").args[1].getReader<int>() == 9);

    int input0 = 4, input1 = 5;
    std::vector<Variable> inputs;
    Assign(inputs, &input0, &input1);
    REQUIRE(inputs[0].getWriter<int>().get() == &input0);
    REQUIRE(core.supposeInput(inputs));
    std::vector<Variable> outputs(1);
    outputs[0].create<int>();
    REQUIRE(core.supposeOutput(outputs));

    REQUIRE(core.linkNode(fase::InputNodeName(), 0, "a", 0));
    REQUIRE(core.linkNode(fase::InputNodeName(), 1, "a", 1));
    REQUIRE(core.linkNode("b", 1, fase::OutputNodeName(), 0));

    REQUIRE(core.run());

    REQUIRE(*outputs[0].getReader<int>() == 81);

    // check deep copy
    auto copied = core;
    std::vector<Variable> copied_inputs(2);
    copied_inputs[0].create<int>(3);
    copied_inputs[1].create<int>(7);
    REQUIRE(copied.supposeInput(copied_inputs));
    std::vector<Variable> copied_outputs(1);
    copied_outputs[0].create<int>();
    REQUIRE(copied.supposeOutput(copied_outputs));

    REQUIRE(copied.run());

    REQUIRE(*copied_outputs[0].getReader<int>() == 100);

    REQUIRE(*outputs[0].getReader<int>() == 81);

    REQUIRE(inputs[0].getWriter<int>().get() == &input0);
    input0 = 2;
    input1 = 6;

    REQUIRE(core.newNode("c"));
    REQUIRE(core.allocateFunc("square", "c"));

    REQUIRE_FALSE(core.linkNode("b", 2, "c", 0));
    REQUIRE(core.linkNode("b", 1, "c", 0));
    REQUIRE(core.linkNode("c", 1, fase::OutputNodeName(), 0));

    REQUIRE(core.run());
    REQUIRE(*outputs[0].getReader<int>() == 64 * 64);

    *copied_inputs[0].getWriter<int>() = 1;
    *copied_inputs[1].getWriter<int>() = 4;
    REQUIRE(copied.supposeInput(copied_inputs));

    REQUIRE(copied.run());
    REQUIRE(*copied_outputs[0].getReader<int>() == 25);
}
