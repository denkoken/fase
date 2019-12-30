#include "catch.hpp"

#include "fase2/fase.h"
using namespace fase;

namespace {

unsigned f(int a, long& b) {
    b += 1;
    return unsigned(a + 1) * unsigned(b);
}

} // namespace

TEST_CASE("UnivFuncGenerator (Ret is not void) test") {
    auto worker = fase::UnivFuncGenerator<unsigned(int, long&)>::Gen(
            []() -> std::function<unsigned(int, long&)> { return f; });

    std::vector<fase::Variable> vs(3);

    vs[0].create<int>(5);
    vs[1].create<long>(10l);
    vs[2].create<unsigned>(0u);

    Report report;

    worker(vs, nullptr);
    REQUIRE(*vs[0].getReader<int>() == 5);
    REQUIRE(*vs[1].getReader<long>() == 11l);
    REQUIRE(*vs[2].getReader<unsigned>() == 66u);
    worker(vs, &report);
    REQUIRE(*vs[0].getReader<int>() == 5);
    REQUIRE(*vs[1].getReader<long>() == 12l);
    REQUIRE(*vs[2].getReader<unsigned>() == 72u);

    vs.emplace_back();
    vs[3].create<int>(9);

    try {
        worker(vs, nullptr);
        REQUIRE(false);
    } catch (std::exception& e) {
    }
}

TEST_CASE("UnivFuncGenerator test") {
    auto buf = [](int a, float& b, double&& c) {
        a++;
        b += 2.f;
        return a * int(b) + c;
    };

    auto worker = fase::UnivFuncGenerator<void(int, float&, double&&)>::Gen(
            [=]() -> std::function<void(int, float&, double&&)> {
                return buf;
            });

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
