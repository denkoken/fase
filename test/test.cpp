#define CATCH_CONFIG_MAIN  // Define main()
#include "catch.hpp"

#include "fase.h"

using fase::FunctionNode;
using fase::StandardFunction;
using fase::Variable;

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

class Add : public FunctionNode {
public:
    FunctionNode *build(const std::vector<Variable *> &args) {
        a = args[0]->getReader<int>();
        b = args[1]->getReader<int>();
        c = args[2]->getWriter<int>();
        return this;
    }

    void apply() { *c = *a + *b; }

private:
    std::shared_ptr<int> a, b, c;
};

TEST_CASE("Variable test") {

    SECTION("Create") {
        Variable v = TestClass();
        REQUIRE(v.getReader<TestClass>()->isMoved());
    }

    SECTION("Create by copy") {
        TestClass a;
        Variable v = a;
        REQUIRE(v.getReader<TestClass>()->isCopied());
        REQUIRE(&*v.getReader<TestClass>() != &a);  // Not equal because copied.
    }

    SECTION("Create by move") {
        TestClass a;
        Variable v = std::move(a);
        REQUIRE(v.getReader<TestClass>()->isMoved());
    }

    SECTION("Copy") {
        Variable a = TestClass();
        Variable b = TestClass();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        a = b;
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
        Variable b = a.clone<TestClass>();
        a.getReader<TestClass>()->clear();
        b.getReader<TestClass>()->clear();
        REQUIRE(&(*a.getReader<TestClass>()) != &(*b.getReader<TestClass>()));
        REQUIRE(a.getReader<TestClass>()->isNone());
        REQUIRE(b.getReader<TestClass>()->isNone());
    }

}

TEST_CASE("FunctionNode test") {

    std::vector<FunctionNode *> fs;
    StandardFunction<int, int &, float> func = [](int a, int &b, float c) {
        (void)a;
        b += c;
    };
    Variable v1 = 1, v2 = 2, v3 = 3.f, v4;

    SECTION("Dynamic build") {
        fs.push_back(func.build({&v1, &v2, &v3}));
        fs.back()->apply();
        REQUIRE(*v2.getReader<int>() == 5);  // 2 + 3
    }

    SECTION("Static build") {
        fs.push_back(func.build(v1, v2, v3));
        fs.back()->apply();
        assert(*v2.getReader<int>() == 8);  // 5 + 3
    }

    SECTION("Inherited function") {
        Add add;
        add.build({&v1, &v2, &v4});
        fs.push_back(&add);
        fs.back()->apply();
        assert(*v4.getReader<int>() == 9);  // 1 + 8
    }

    SECTION("Call by loop") {
        REQUIRE_NOTHROW([&](){
            for (auto &f : fs) {
                f->apply();
            }
        }());
    }

}

TEST_CASE("WrongTypeCast test") {
    Variable test_class = TestClass();
    try {
        test_class.getReader<float>();
        REQUIRE(false);
    } catch (fase::WrongTypeCast &e) {
        REQUIRE(*e.casted_type == typeid(TestClass));
        REQUIRE(*e.cast_type == typeid(float));
    }
}
