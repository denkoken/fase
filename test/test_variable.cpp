#include <catch2/catch.hpp>

#include "fase2/fase.h"
using namespace fase;

class TestClass {
public:
    enum class Status { NONE, MAKED, COPIED, MOVED };

    TestClass() : status(Status::MAKED) {}
    TestClass(const TestClass&) : status(Status::COPIED) {}
    TestClass(TestClass&&) : status(Status::MOVED) {}
    TestClass& operator=(const TestClass&) {
        status = Status::COPIED;
        return *this;
    }
    TestClass& operator=(TestClass&&) {
        status = Status::MOVED;
        return *this;
    }

    bool isNone() const {
        return status == Status::NONE;
    }
    bool isMaked() const {
        return status == Status::MAKED;
    }
    bool isCopied() const {
        return status == Status::COPIED;
    }
    bool isMoved() const {
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
        a.getWriter<TestClass>()->clear();
        b.getWriter<TestClass>()->clear();
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
        a.getWriter<TestClass>()->clear();
        b.getWriter<TestClass>()->clear();
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
        a.getWriter<TestClass>()->clear();
        b.getWriter<TestClass>()->clear();
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
        a.getWriter<TestClass>()->clear();
        b.getWriter<TestClass>()->clear();
        a = std::move(b);
        REQUIRE(a.getReader<TestClass>()->isNone());
    }

    SECTION("Clone") {
        Variable a;
        a.create<TestClass>();
        a.getWriter<TestClass>()->clear();
        Variable b = a.clone();
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isCopied());
    }

    SECTION("Empry Ref : Asign") {
        Variable a(typeid(TestClass));
        Variable b = a.ref();
        Variable c;
        c.create<TestClass>();
        c.getWriter<TestClass>()->clear();
        b = c;
        REQUIRE(!a);
        REQUIRE(b.getReader<TestClass>()->isCopied());
        REQUIRE(c.getReader<TestClass>()->isNone());
    }

    SECTION("Empry Ref : Copy") {
        Variable a(typeid(TestClass));
        Variable b = a.ref();
        Variable c;
        c.create<TestClass>();
        c.getWriter<TestClass>()->clear();
        c.copyTo(b);
        REQUIRE(a.getReader<TestClass>().get() ==
                b.getReader<TestClass>().get());
        REQUIRE(a.getReader<TestClass>()->isCopied());
        REQUIRE(b.getReader<TestClass>()->isCopied());
        REQUIRE(c.getReader<TestClass>()->isNone());
    }

    SECTION("void Type Empry Ref") {
        Variable a;
        Variable b = a.ref();
        Variable c;
        c.create<TestClass>();
        try {
            c.copyTo(b);
            REQUIRE(false);
        } catch (WrongTypeCast& e) {
            REQUIRE(e.cast_type == typeid(void));
            REQUIRE(e.casted_type == typeid(TestClass));
        }
        REQUIRE(!a);
        REQUIRE(!b);
    }

    SECTION("No Manage Variable") {
        int a = 123;
        {
            Variable v(&a);
            REQUIRE(*v.getReader<int>() == 123);
            REQUIRE(v.getReader<int>().get() == &a);
            *v.getWriter<int>() = 2;
        }
        REQUIRE(a == 2);
    }

    SECTION("assignedAs") {
        Variable a(typeid(TestClass));
        Variable b(typeid(int));
        Variable c = std::make_unique<TestClass>();
        Variable d;

        TestClass tc;
        tc.clear();

        a.assignedAs(tc);
        REQUIRE(a.getReader<TestClass>()->isCopied());

        TestClass tc_m;
        tc_m.clear();
        a.assignedAs(std::move(tc_m));
        REQUIRE(a.getReader<TestClass>()->isMoved());

        try {
            b.assignedAs(tc);
            REQUIRE(false);
        } catch (WrongTypeCast& e) {
            REQUIRE(e.cast_type == typeid(TestClass));
            REQUIRE(e.casted_type == typeid(int));
        } catch (...) {
            REQUIRE(false);
        }

        c.assignedAs(tc);
        REQUIRE(c.getReader<TestClass>()->isCopied());

        d.assignedAs(tc);
        REQUIRE(d.getReader<TestClass>()->isCopied());
    }

    SECTION("WrongTypeCast") {
        Variable test_class;
        test_class.create<TestClass>();
        try {
            test_class.getReader<float>();
            REQUIRE(false);
        } catch (WrongTypeCast& e) {
            REQUIRE(e.casted_type == typeid(TestClass));
            REQUIRE(e.cast_type == typeid(float));
        }
    }
}
