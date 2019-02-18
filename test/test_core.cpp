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
                UnivFuncGenerator<const int&, const int&, int&>::Gen(Add);

        std::vector<Variable> default_args(3);

        default_args[0].create<int>(1);
        default_args[1].create<int>(2);
        default_args[2].create<int>(0);

        REQUIRE(core.addUnivFunc(univ_add, "add", std::move(default_args)));
    }

    {
        auto univ_sq = UnivFuncGenerator<const int&, int&>::Gen(Square);

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

    {
        std::vector<Variable> inputs(2);
        inputs[0].create<int>(4);
        inputs[1].create<int>(5);
        REQUIRE(core.supposeInput(inputs));
    }
    std::vector<Variable> outputs(1);
    outputs[0].create<int>();
    REQUIRE(core.supposeOutput(outputs));

    REQUIRE(core.linkNode(fase::InputNodeName(), 0, "a", 0));
    REQUIRE(core.linkNode(fase::InputNodeName(), 1, "a", 1));
    REQUIRE(core.linkNode("b", 1, fase::OutputNodeName(), 0));

    REQUIRE(core.run());

    REQUIRE(*outputs[0].getReader<int>() == 81);
}
