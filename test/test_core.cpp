#include "catch.hpp"

#include "fase.h"
using namespace fase;

#include <sstream>


template <typename... Args>
struct NArgs {
    size_t N = sizeof...(Args);
};

template <typename... Args>
struct NArgs<void(Args...)> {
    size_t N = sizeof...(Args);
};

template <std::size_t N>
void extractArgExprs(const std::string &raw_types,
                     std::array<std::string, N>& reprs) {
    const char delim = ',';
    std::stringstream ss(raw_types);
    std::string item;
    size_t idx = 0;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) {
            reprs[idx] = item;
            idx += 1;
            assert(idx <= N);
        }
    }
}

#define FaseCoreAddFunctionBuilder(core, func, arg_types, ...) [&](){   \
    std::array<std::string, NArgs<void arg_types>{}.N>  arg_reprs;      \
    extractArgExprs(#arg_types, arg_reprs);                             \
    return core.addFunctionBuilder(#func,                               \
                                   std::function<void arg_types>(func), \
                                   arg_reprs, {__VA_ARGS__});           \
}()


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

    SECTION("1") {
        FaseCoreAddFunctionBuilder(core, Add, (const int&, const int&, int&),
                                   1, 2);
        FaseCoreAddFunctionBuilder(core, Print, (const int&), 111);

        REQUIRE(core.makeNode("add1", "Add"));
        REQUIRE(core.makeNode("add2", "Add"));
        REQUIRE(core.makeNode("add3", "Add"));
        REQUIRE(core.makeNode("print1", "Print"));
        REQUIRE(core.makeNode("print2", "Print"));

        REQUIRE(core.linkNode("add1", 2, "add2", 0));
        REQUIRE(core.linkNode("add2", 2, "add3", 0));
        REQUIRE(core.linkNode("add3", 2, "print1", 0));

        REQUIRE(core.setNodeArg("print2", 0, 100));

        REQUIRE(core.build());
        REQUIRE(core.run());
    }
}
