
#include "fase.h"

#include <cassert>

using fase::FunctionNode;
using fase::StandardFunction;
using fase::Variable;

class TestClass {
public:
    TestClass() : maked(true), copied(false), moved(false) {}
    TestClass(const TestClass &) : maked(false), copied(true), moved(false) {}
    TestClass(TestClass &&) : maked(false), copied(false), moved(true) {}
    TestClass &operator=(TestClass &&) {
        this->copied = true;
        this->maked = this->moved = false;
        return *this;
    }
    TestClass &operator=(const TestClass &) {
        this->moved = true;
        this->maked = this->copied = false;
        return *this;
    }
    void test() { printf("success!\n"); }

    bool isMaked() { return maked; }
    bool isCopied() { return copied; }
    bool isMoved() { return moved; }

private:
    bool maked;
    bool copied;
    bool moved;
};

class Add : public FunctionNode {
public:
    void build(const std::vector<Variable *> &args) {
        if (args.size() != 3) {
            std::cerr << "Invalid arguments" << std::endl;
            return;
        }
        a = args[0]->getReader<int>();
        b = args[1]->getReader<int>();
        c = args[2]->getWriter<int>();
    }

    void apply() { *c = *a + *b; }

private:
    std::shared_ptr<int> a, b, c;
};

int main() {
    Variable v3;

    Variable a = TestClass();
    assert(a.getReader<TestClass>()->isMoved());
    TestClass buf;

    Variable b = buf;
    assert(b.getReader<TestClass>()->isCopied());

    a = b;

    b.getReader<TestClass>()->test();
    a.getReader<TestClass>()->test();

    a = b;
    a.getReader<TestClass>()->test();

    b = std::move(buf);
    assert(b.getReader<TestClass>()->isMoved());
    b.getReader<TestClass>()->test();

    {
        StandardFunction<int, int &, float> func = [](int a, int &b, float c) {
            printf("a: %d, b: %d\n", a, b);
            b += c;
            printf("a: %d, b: %d\n", a, b);
        };
        FunctionNode *pf = &func;
        Variable v1 = 1, v2 = 2, v3 = 3.f, v4;
        pf->build({&v1, &v2, &v3});
        pf->apply();
        pf->apply();
        assert(*v2.getReader<int>() == 8);

        try {
            pf->build({&v1, &v2, &v3, &v4});
            assert(false);
        } catch (fase::InvalidArgN &e) {
            assert(e.expectedN == 3 && e.inputN == 4);
        }

        Add add;
        add.build({&v1, &v2, &v4});

        std::vector<FunctionNode *> fs;
        fs.push_back(&func);
        fs.push_back(&add);

        for (auto &f : fs) {
            f->apply();
        }
    }

    {
        Variable test_class = TestClass();
        try {
            test_class.getReader<float>();
            assert(false);
        } catch (fase::WrongTypeCast &e) {
            assert(*e.casted_type == typeid(TestClass) &&
                   *e.cast_type == typeid(float));
        }
    }

    Add add;
    FunctionNode *node = &add;
    {
        // Create with variable creation
        Variable v1 = 123, v2 = 456;

        // Correct cast
        std::cout << *(v1.getReader<int>()) << std::endl;
        std::cout << *(v2.getReader<int>()) << std::endl;

        node->build({&v1, &v2, &v3});
    }
    node->apply();
    assert(*(v3.getReader<int>()) == 579);
    std::cout << *(v3.getReader<int>()) << std::endl;
    return 0;
}
