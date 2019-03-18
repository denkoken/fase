#include "catch.hpp"

#include <fase2/fase.h>
#include <fase2/stdparts.h>

using namespace fase;

// clang-format off

FaseAutoAddingUnivFunction(Add,
void Add(const int& a, const int& b, int& dst) {
    dst = a + b;
});

FaseAutoAddingUnivFunction(Square,
void Square(const int& in, int& dst) {
    dst = in * in;
});

void Times(const int& a, const int& b, int& dst) {
    dst = a * b;
}

// clang-format on

class BareCore : public PartsBase {
public:
    auto getCoreManager() {
        return getWriter();
    }
};

TEST_CASE("Fase test") {
    Fase<BareCore> app;
    FaseAddUnivFunction(Times, (const int&, const int&, int&),
                        ("a", "b", "dst"), app, "dst = a * b", {1, 1, 1});

    auto [guard, pcm] = app.getCoreManager();
    auto& cm = *pcm;
    REQUIRE(cm["test"].newNode("a"));
    REQUIRE(cm["test"].allocateFunc("Add", "a"));
    REQUIRE(cm["test"].allocateFunc("Square", "a"));
    REQUIRE(cm["test"].allocateFunc("Times", "a"));
}
