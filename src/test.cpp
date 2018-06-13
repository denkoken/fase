
#include "fase.h"

using fase::FunctionNode;
using fase::StandardFunction;
using fase::Variable;

class TestClass {
public:
    TestClass() { printf("new instance was made.\n"); }
    TestClass(const TestClass &) { printf("copy instance was made.\n"); }
    TestClass(TestClass &&) { printf("moved instance was made.\n"); }
    TestClass &operator=(TestClass &&) {
        printf("moved instance was made.\n");
        return *this;
    }
    TestClass &operator=(const TestClass &) {
        printf("copy instance was made.\n");
        return *this;
    }
    void test() { printf("success!\n"); }
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
    TestClass buf;

    Variable b = buf;

    a = b;

    b.getReader<TestClass>()->test();
    a.getReader<TestClass>()->test();

    a = b;
    a.getReader<TestClass>()->test();

    b = std::move(buf);
    b.getReader<TestClass>()->test();

    {
        StandardFunction<int, float &, int> func = [](int a, float &b, int c) {
            printf("a: %d, b: %f\n", a, b);
            b += c;
        };
        FunctionNode *pf = &func;
        Variable v1 = 1, v2 = 2.f, v3 = 3;
        pf->build({&v1, &v2, &v3});
        pf->apply();
        pf->apply();
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
    std::cout << *(v3.getReader<int>()) << std::endl;
    return 0;
}
