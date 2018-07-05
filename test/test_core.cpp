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
    // Remove '(' and ')'
    auto l_par_idx = raw_types.find('(');
    auto r_par_idx = raw_types.rfind(')');
    assert(l_par_idx != std::string::npos && r_par_idx != std::string::npos);
    std::string types = raw_types.substr(l_par_idx + 1,
                                         r_par_idx - l_par_idx - 1);

    // Split by ','
    std::stringstream ss(types);
    std::string item;
    size_t idx = 0;
    while (std::getline(ss, item, ',')) {
        // Remove extra spaces
        size_t l_sp_idx = 0;
        while (item[l_sp_idx] == ' ') l_sp_idx++;
        item = item.substr(l_sp_idx);
        size_t r_sp_idx = item.size() - 1;
        while (item[r_sp_idx] == ' ') r_sp_idx--;
        item = item.substr(0, r_sp_idx + 1);

        // Register
        reprs[idx] = item;
        idx += 1;
        assert(idx <= N);
    }
}

#define FaseAddFunctionBuilder(core, func, arg_types, ...) [&](){       \
    std::array<std::string, NArgs<void arg_types>{}.N>  arg_reprs;      \
    extractArgExprs(#arg_types, arg_reprs);                             \
    return core.addFunctionBuilder(#func,                               \
                                   std::function<void arg_types>(func), \
                                   arg_reprs, {__VA_ARGS__});           \
}()


void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
}

void Square(const int& in, int& dst) {
    dst = in * in;
}

TEST_CASE("Core test") {
    FaseCore core;

    SECTION("Build basic") {
        REQUIRE(FaseAddFunctionBuilder(core, Add,
                                       (const int&, const int&, int&)));
        REQUIRE(FaseAddFunctionBuilder(core, Square,
                                       (const int&, int&)));
        REQUIRE(core.makeNode("add1", "Add"));
        REQUIRE(core.makeNode("square1", "Square"));
        REQUIRE(core.linkNode("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 0, 10));
        REQUIRE(core.setNodeArg("add1", 1, 20));
        REQUIRE(core.build());
        REQUIRE(core.run());
        REQUIRE(core.getOutput<int>("square1", 1) == 900);  // (10 + 20) ** 2
        std::cout << core.genNativeCode() << std::endl;
    }

    SECTION("Build with default argument") {
        REQUIRE(FaseAddFunctionBuilder(core, Add,
                                       (const int&, const int&, int&), 30));
        REQUIRE(FaseAddFunctionBuilder(core, Square, (const int&, int&)));
        REQUIRE(core.makeNode("add1", "Add"));
        REQUIRE(core.makeNode("square1", "Square"));
        REQUIRE(core.linkNode("add1", 2, "square1", 0));
        REQUIRE(core.setNodeArg("add1", 1, 20));
        REQUIRE(core.build());
        REQUIRE(core.run());
        REQUIRE(core.getOutput<int>("square1", 1) == 2500);  // (30 + 20) ** 2
    }

    SECTION("Duplicate AddFunctionBuilder") {
        REQUIRE(FaseAddFunctionBuilder(core, Add,
                                       (const int&, const int&, int&)));
        REQUIRE_FALSE(FaseAddFunctionBuilder(core, Add,
                                             (const int&, const int&, int&)));
    }

    SECTION("Detect invalid node") {
        REQUIRE(FaseAddFunctionBuilder(core, Add,
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
