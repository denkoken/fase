#include "catch.hpp"

#include "fase.h"
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

    bool isNone() { return status == Status::NONE; }
    bool isMaked() { return status == Status::MAKED; }
    bool isCopied() { return status == Status::COPIED; }
    bool isMoved() { return status == Status::MOVED; }

    void clear() { status = Status::NONE; }

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

    SECTION("Create by temporary value") {
        Variable v = TestClass();
        REQUIRE(v.isSameType<TestClass>());
        REQUIRE(v.getReader<TestClass>()->isMoved());
    }

    SECTION("Create by instance copy") {
        TestClass a;
        Variable v = a;
        REQUIRE(a.isMaked());
        REQUIRE(v.isSameType<TestClass>());
        REQUIRE(v.getReader<TestClass>()->isMoved());
        REQUIRE(&*v.getReader<TestClass>() != &a);  // Not equal (copied)
    }

    SECTION("Create by instance move") {
        TestClass a;
        Variable v = std::move(a);
        REQUIRE(v.isSameType<TestClass>());
        REQUIRE(v.getReader<TestClass>()->isMoved());
        REQUIRE(&*v.getReader<TestClass>() != &a);  // Not equal (copied)
    }

    SECTION("Create by set") {
        std::shared_ptr<TestClass> a = std::make_shared<TestClass>();
        Variable v = a;
        REQUIRE(v.isSameType<TestClass>());
        REQUIRE(v.getReader<TestClass>()->isMaked());  // Not copied
        REQUIRE(&*v.getReader<TestClass>() == &*a);    // Equal (pointer copied)
    }

    SECTION("Copy") {
        Variable a = TestClass();
        Variable b = TestClass();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = b;
        REQUIRE(a.isSameType(b));
        REQUIRE(&(*a.getReader<TestClass>()) == &(*b.getReader<TestClass>()));
        // No copy/move is happened because shared_ptr is copied.
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

    SECTION("Move") {
        Variable a = TestClass();
        Variable b = TestClass();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = std::move(b);
        REQUIRE(a.getReader<TestClass>()->isNone());
    }

    SECTION("Clone") {
        Variable a = TestClass();
        Variable b = a.clone();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

    SECTION("WrongTypeCast") {
        Variable test_class = TestClass();
        try {
            test_class.getReader<float>();
            REQUIRE(false);
        } catch (WrongTypeCast &e) {
            REQUIRE(*e.casted_type == typeid(TestClass));
            REQUIRE(*e.cast_type == typeid(float));
        }
    }
}
