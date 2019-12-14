#include "catch.hpp"

#include "fase2/fase.h"
using namespace fase;

TEST_CASE("FunctionBuilder test") {
    auto buf = [](int a, float& b, double&& c) {
        a++;
        b += 2.f;
        return a * int(b) + c;
    };

    auto worker_gen = fase::UnivFuncGenerator<int, float&, double&&>::Gen(
            [=]() -> std::function<void(int, float&, double&&)> {
                return buf;
            });

    auto worker = worker_gen;

    std::vector<fase::Variable> vs(3);

    vs[0].create<int>(5);
    vs[1].create<float>(3.5f);
    vs[2].create<double>(5.3);

    Report report;

    worker(vs, nullptr);
    REQUIRE(*vs[0].getReader<int>() == 5);
    REQUIRE(*vs[1].getReader<float>() == 5.5f);
    REQUIRE(*vs[2].getReader<double>() == 5.3);
    worker(vs, &report);
    REQUIRE(*vs[0].getReader<int>() == 5);
    REQUIRE(*vs[1].getReader<float>() == 7.5f);
    REQUIRE(*vs[2].getReader<double>() == 5.3);

    vs.emplace_back();
    vs[3].create<int>(9);

    try {
        worker(vs, nullptr);
        REQUIRE(false);
    } catch (std::exception& e) {
    }
}
