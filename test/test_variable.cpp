#include "catch.hpp"

#include "fase2/fase.h"
using namespace fase;

class TestClass {
public:
    enum class Status { NONE, MAKED, COPIED, MOVED };

    TestClass() : status(Status::MAKED) {}
    TestClass(const TestClass &) : status(Status::COPIED) {}
    TestClass(TestClass &&) : status(Status::MOVED) {}
    TestClass &operator=(const TestClass &) {
        status = Status::COPIED;
        return *this;
    }
    TestClass &operator=(TestClass &&) {
        status = Status::MOVED;
        return *this;
    }

    bool isNone() {
        return status == Status::NONE;
    }
    bool isMaked() {
        return status == Status::MAKED;
    }
    bool isCopied() {
        return status == Status::COPIED;
    }
    bool isMoved() {
        return status == Status::MOVED;
    }

    void clear() {
        status = Status::NONE;
    }

private:
    Status status;
};

TEST_CASE("Variable test") {
    SECTION("Create directly") {
        Variable v;
        v.create<TestClass>();
        REQUIRE(v.isSameType<TestClass>());
        REQUIRE(v.getReader<TestClass>()->isMaked());
    }

    SECTION("Copy") {
        Variable a;
        a.create<TestClass>();
        Variable b;
        b.create<TestClass>();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = b;
        REQUIRE(a.isSameType(b));
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isCopied());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

    SECTION("Clone copy") {
        Variable a;
        a.create<TestClass>();
        Variable b;
        b.create<TestClass>();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = b.clone();
        REQUIRE(a.isSameType(b));
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isCopied());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

    SECTION("Ref Copy") {
        Variable a;
        a.create<TestClass>();
        Variable b;
        b.create<TestClass>();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = b.ref();
        REQUIRE(a.isSameType(b));
        REQUIRE(&(*a.getReader<TestClass>()) == &(*b.getReader<TestClass>()));
        // No copy/move is happened because shared_ptr is copied.
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

    SECTION("Move") {
        Variable a;
        a.create<TestClass>();
        Variable b;
        b.create<TestClass>();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = std::move(b);
        REQUIRE(a.getReader<TestClass>()->isNone());
    }

    SECTION("Clone") {
        Variable a;
        a.create<TestClass>();
        a.getReader<TestClass>()->clear();
        Variable b = a.clone();
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isCopied());
    }

    SECTION("WrongTypeCast") {
        Variable test_class;
        test_class.create<TestClass>();
        try {
            test_class.getReader<float>();
            REQUIRE(false);
        } catch (WrongTypeCast &e) {
            REQUIRE(e.casted_type == typeid(TestClass));
            REQUIRE(e.cast_type == typeid(float));
        }
    }
}
