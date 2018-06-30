#define CATCH_CONFIG_MAIN  // Define main()
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
        REQUIRE_NOTHROW([&](){
            std::vector<FunctionBuilderBase *> builders =
                { &builder, &builder2, &builder };
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
