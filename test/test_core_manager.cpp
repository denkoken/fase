#include "catch.hpp"

#include <iostream>
#include <memory>

#include "fase2/constants.h"
#include "fase2/fase.h"

using namespace fase;

static void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

static void Square(const int& in, int& dst) {
    dst = in * in;
}

TEST_CASE("Core Manager test") {
    CoreManager cm;

    {
        auto univ_add =
                UnivFuncGenerator<const int&, const int&, int&>::Gen(Add);

        std::vector<Variable> default_args(3);

        default_args[0].create<int>(1);
        default_args[1].create<int>(2);
        default_args[2].create<int>(0);

        REQUIRE(cm.addUnivFunc(univ_add, "add", std::move(default_args),
                               {"a", "b", "dst"}, "dst := a + b"));
    }

    {
        auto univ_sq = UnivFuncGenerator<const int&, int&>::Gen(Square);

        std::vector<Variable> default_args(2);

        default_args[0].create<int>(4);
        default_args[1].create<int>(0);

        REQUIRE(cm.addUnivFunc(univ_sq, "square", std::move(default_args),
                               {"in", "dst"}, "dst := in * in"));
    }

    {
        auto lambda = [pc = std::make_unique<float>(3.5)](int in, int& dst) {
            dst = float(in) * *pc;
        };
        auto univ_lambda = UnivFuncGenerator<int, int&>::Gen(std::move(lambda));

        std::vector<Variable> default_args = {std::make_shared<int>(3),
                                              std::make_shared<int>()};

        REQUIRE(cm.addUnivFunc(univ_lambda, "lambda", std::move(default_args),
                               {"in", "dst"}, "dst := C * in"));
    }
    const std::string kINPUT = fase::InputNodeName();
    const std::string kOUTPUT = fase::OutputNodeName();

    REQUIRE(cm["Pipe1"].supposeInput({"in1", "in2"}));
    REQUIRE(cm["Pipe1"].supposeOutput({"dst"}));

    REQUIRE(cm["Pipe1"].newNode("a"));
    REQUIRE(cm["Pipe1"].newNode("b"));
    REQUIRE(cm["Pipe1"].allocateFunc("add", "a"));
    REQUIRE(cm["Pipe1"].allocateFunc("square", "b"));
    REQUIRE(cm["Pipe1"].smartLink(kINPUT, 0, "a", 0));
    REQUIRE(cm["Pipe1"].smartLink(kINPUT, 1, "a", 1));
    REQUIRE(cm["Pipe1"].smartLink("a", 2, "b", 0));
    REQUIRE(cm["Pipe1"].smartLink("b", 1, kOUTPUT, 0));
    REQUIRE(cm["Pipe1"].run());

    REQUIRE(cm["Pipe2"].newNode("One"));
    REQUIRE(cm["Pipe2"].newNode("l"));
    REQUIRE(cm["Pipe2"].allocateFunc("Pipe1", "One"));
    REQUIRE(cm["Pipe2"].allocateFunc("lambda", "l"));
    REQUIRE(cm["Pipe2"].smartLink("One", 2, "l", 0));

    Variable v = std::make_shared<int>(5);
    Variable v_ = std::make_shared<int>(6);

    REQUIRE(cm["Pipe2"].setArgument("One", 0, v));
    REQUIRE(cm["Pipe2"].setArgument("One", 1, v_));

    REQUIRE(cm["Pipe2"].run());

    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((5 + 6) * (5 + 6))));
}
