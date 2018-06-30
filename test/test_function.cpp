#include "catch.hpp"

#include "fase.h"
using namespace fase;

TEST_CASE("FunctionBuilder test") {
    FunctionBuilder<int, int &, float> builder = [](int a, int &b, float c) {
        (void)a;
        b += c;
    };
    FunctionBuilder<int, int &, float> builder2 = [](int a, int &b, float c) {
        b += (a + c);
    };
    Variable v1 = 1, v2 = 2, v3 = 3.f;

    SECTION("Dynamic build") {
        std::function<void()> f = builder.build({&v1, &v2, &v3});
        REQUIRE(*v2.getReader<int>() == 2);  // 2
        f();
        REQUIRE(*v2.getReader<int>() == 5);  // 2 + 3
        f();
        REQUIRE(*v2.getReader<int>() == 8);  // 5 + 3
    }

    SECTION("Static build") {
        std::function<void()> f = builder.build(&v1, &v2, &v3);
        REQUIRE(*v2.getReader<int>() == 2);  // 2
        f();
        REQUIRE(*v2.getReader<int>() == 5);  // 2 + 3
        f();
        REQUIRE(*v2.getReader<int>() == 8);  // 5 + 3
    }

    SECTION("Call by loop") {
        REQUIRE_NOTHROW([&]() {
            std::vector<FunctionBuilderBase *> builders = {&builder, &builder2,
                                                           &builder};
            REQUIRE(*v2.getReader<int>() == 2);  // 2
            for (auto &b : builders) {
                std::function<void()> f = b->build({&v1, &v2, &v3});
                f();
            }
            REQUIRE(*v2.getReader<int>() == 12);  // 2 + 3 + (1 + 3) + 3
        }());
    }

    SECTION("InvalidArgN") {
        try {
            builder.build({&v1, &v2});
            REQUIRE(false);
        } catch (InvalidArgN &e) {
            REQUIRE(e.input_n == 2);
            REQUIRE(e.expected_n == 3);
        }
    }
}
