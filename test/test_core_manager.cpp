#include <catch2/catch.hpp>

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
                UnivFuncGenerator<void(const int&, const int&, int&)>::Gen(
                        []() -> std::function<void(const int&, const int&,
                                                   int&)> { return Add; });

        std::deque<Variable> default_args(3);

        default_args[0].create<int>(1);
        default_args[1].create<int>(2);
        default_args[2].create<int>(0);

        REQUIRE(cm.addUnivFunc(univ_add, "add", std::move(default_args),
                               {{"a", "b", "dst"},
                                {typeid(int), typeid(int), typeid(int)},
                                {true, true, false},
                                FOGtype::Pure,
                                "",
                                {},
                                "",
                                "dst := a + b"}));
    }

    {
        auto univ_sq = UnivFuncGenerator<void(const int&, int&)>::Gen(
                []() -> std::function<void(const int&, int&)> {
                    return Square;
                });

        std::deque<Variable> default_args(2);

        default_args[0].create<int>(4);
        default_args[1].create<int>(0);

        REQUIRE(cm.addUnivFunc(univ_sq, "square", std::move(default_args),
                               {{"in", "dst"},
                                {typeid(int), typeid(int)},
                                {true, false},
                                FOGtype::Pure,
                                "",
                                {},
                                "",
                                "dst := in * in"}));
    }

    {
        auto lambda = [pc = std::make_shared<float>(3.5)](int in, int& dst) {
            dst = float(in) * *pc;
        };
        auto univ_lambda = UnivFuncGenerator<void(int, int&)>::Gen(
                [&lambda]() -> std::function<void(int, int&)> {
                    return lambda;
                });

        std::deque<Variable> default_args = {std::make_unique<int>(3),
                                             std::make_unique<int>()};

        REQUIRE(cm.addUnivFunc(univ_lambda, "lambda", std::move(default_args),
                               {{"in", "dst"},
                                {typeid(int), typeid(int)},
                                {true, false},
                                FOGtype::Pure,
                                "",
                                {},
                                "",
                                "dst := in * in"}));
    }
    const std::string kINPUT = fase::InputNodeName();
    const std::string kOUTPUT = fase::OutputNodeName();

    REQUIRE(cm["Pipe1"].supposeInput({"in1", "in2"}));
    REQUIRE(cm["Pipe1"].supposeOutput({"dst"}));

    REQUIRE(cm["Pipe1"].newNode("a"));
    REQUIRE(cm["Pipe1"].newNode("b"));
    REQUIRE(cm["Pipe1"].allocateFunc("add", "a"));
    REQUIRE(cm["Pipe1"].allocateFunc("square", "b"));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink(kINPUT, 0, "a", 0));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink(kINPUT, 1, "a", 1));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink("a", 2, "b", 0));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink("b", 1, kOUTPUT, 0));

    REQUIRE(cm["Pipe2"].newNode("One"));
    REQUIRE(cm["Pipe2"].newNode("l"));
    REQUIRE(cm["Pipe2"].allocateFunc("Pipe1", "One"));
    REQUIRE(cm["Pipe2"].allocateFunc("lambda", "l"));
    REQUIRE(LinkNodeError::None == cm["Pipe2"].smartLink("One", 2, "l", 0));

    Variable var = std::make_unique<int>(5);
    Variable var_ = std::make_unique<int>(6);

    REQUIRE(cm["Pipe2"].setArgument("One", 0, var));
    REQUIRE(cm["Pipe2"].setArgument("One", 1, var_));

    REQUIRE(cm["Pipe2"].run());

    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((5 + 6) * (5 + 6))));

    struct Counter {
        int count = 0;
        void operator()(int& dst) {
            dst = count++;
        }
    };

    auto univ_counter = UnivFuncGenerator<void(int&)>::Gen(
            []() -> std::function<void(int&)> { return Counter{}; });

    REQUIRE(cm.addUnivFunc(univ_counter, "counter", {std::make_unique<int>(0)},
                           {{"count"},
                            {typeid(int)},
                            {false},
                            FOGtype::IndependingClass,
                            "",
                            {},
                            "",
                            "counter."}));
    REQUIRE(cm["Pipe1"].newNode("c"));
    REQUIRE(cm["Pipe1"].allocateFunc("counter", "c"));
    REQUIRE(cm["Pipe1"].supposeInput({"in1"}));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink(kINPUT, 0, "a", 0));
    REQUIRE(LinkNodeError::None == cm["Pipe1"].smartLink("c", 0, "a", 1));
    REQUIRE(cm["Pipe1"].run());

    REQUIRE(*cm["Pipe1"].getNodes().at("c").args[0].getReader<int>() == 0);
    REQUIRE(cm["Pipe1"].run());
    REQUIRE(*cm["Pipe1"].getNodes().at("c").args[0].getReader<int>() == 1);
    REQUIRE(*cm["Pipe1"].getNodes().at("a").args[0].getReader<int>() == 1);
    REQUIRE(*cm["Pipe1"].getNodes().at("b").args[1].getReader<int>() == 4);

    REQUIRE(LinkNodeError::None == cm["Pipe2"].smartLink("One", 1, "l", 0));
    var = std::make_unique<int>(3);
    REQUIRE(cm["Pipe2"].setArgument("One", 0, var));
    REQUIRE(cm["Pipe2"].run());

    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((3 + 2) * (3 + 2))));

    REQUIRE(cm["Pipe1"].allocateFunc("counter", "c"));
    REQUIRE(cm["Pipe2"].run());
    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((3 + 0) * (3 + 0))));
    REQUIRE(cm["Pipe2"].run());
    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((3 + 1) * (3 + 1))));
    REQUIRE(cm["Pipe2"].run());
    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((3 + 2) * (3 + 2))));

    { // exportPipe test.
        auto exported = cm.exportPipe("Pipe1");
        int result, input = 3;
        std::deque<Variable> vs;
        Assign(vs, &input, &result);
        exported(vs);
        REQUIRE(result == (3 + 3) * (3 + 3));
        exported(vs);
        REQUIRE(result == (3 + 4) * (3 + 4));
        exported(vs);
        exported(vs);
        exported(vs);
        exported(vs);
        exported.reset();
        exported(vs);
        REQUIRE(result == (3 + 3) * (3 + 3));
        exported(vs);
        exported(vs);
        exported(vs);
        for (Variable& v : vs) {
            v.free();
        }
    }

    { // exportPipe test, with pipe dependence.
        REQUIRE(cm["Pipe2"].supposeInput({"in1"}));
        REQUIRE(cm["Pipe2"].supposeOutput({"dst"}));
        REQUIRE(LinkNodeError::None ==
                cm["Pipe2"].smartLink(kINPUT, 0, "One", 0));
        REQUIRE(LinkNodeError::None ==
                cm["Pipe2"].smartLink("l", 1, kOUTPUT, 0));
        auto exported = cm.exportPipe("Pipe2");
        int result, input = 1;
        std::deque<Variable> vs;
        Assign(vs, &input, &result);
        exported(vs);
        REQUIRE(result == int(3.5f * (1 + 3) * (1 + 3)));
        exported(vs);
        REQUIRE(result == int(3.5f * (1 + 4) * (1 + 4)));
        exported(vs);
        exported(vs);
        exported.reset();
        exported(vs);
        REQUIRE(result == int(3.5f * (1 + 3) * (1 + 3)));
        exported(vs);
        exported(vs);
        exported(vs);
        for (Variable& v : vs) {
            v.free();
        }
    }
    REQUIRE(cm["Pipe2"].run());
    REQUIRE(*cm["Pipe2"].getNodes().at("l").args[1].getReader<int>() ==
            int(3.5f * ((3 + 3) * (3 + 3))));

    { // check DependenceTree.
        REQUIRE_FALSE(cm["Pipe1"].allocateFunc("Pipe2", "a"));

        REQUIRE(cm["Pipe3"].newNode("p2"));
        REQUIRE(cm["Pipe3"].allocateFunc("Pipe2", "p2"));

        REQUIRE_FALSE(cm["Pipe1"].allocateFunc("Pipe3", "a"));

        REQUIRE(cm["Pipe3"].allocateFunc("square", "p2"));

        REQUIRE(cm["Pipe1"].allocateFunc("Pipe3", "a"));

        REQUIRE_FALSE(cm["Pipe3"].allocateFunc("Pipe2", "p2"));
        REQUIRE(cm["Pipe1"].delNode("a"));
        REQUIRE(cm["Pipe3"].allocateFunc("Pipe2", "p2"));
    }
}
