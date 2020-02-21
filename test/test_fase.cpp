#include <catch2/catch.hpp>

#include <fase2/fase.h>
#include <fase2/stdparts.h>

using namespace fase;

namespace {

// clang-format off

FaseAutoAddingUnivFunction(Add,
void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
})

FaseAutoAddingUnivFunction(Square,
void Square(const int& in, int& dst) {
    dst = in * in;
})

int Times(const int& a, const int& b) {
    return a * b;
}

// clang-format on

} // namespace

class BareCore : public PartsBase {
public:
    auto getCoreManager() {
        return getAPI()->getWriter();
    }
};

TEST_CASE("Fase test") {
    Fase<BareCore> app;
    FaseAddUnivFunction(Times, int(const int&, const int&), ("a", "b"), app,
                        "dst = a * b", {1, 1});

    auto [guard, pcm] = app.getCoreManager();
    auto& cm = *pcm;
    REQUIRE(cm["test"].newNode("a"));
    REQUIRE(cm["test"].allocateFunc("Add", "a"));
    REQUIRE(cm["test"].allocateFunc("Square", "a"));
    REQUIRE(cm["test"].allocateFunc("Times", "a"));
}
