
#ifndef FUNCS_H_20191210
#define FUNCS_H_20191210

#include <fase2/fase.h>

namespace {

// clang-format off

FaseAutoAddingUnivFunction(Add,
void Add(const float& a, const float& b, float& dst) {
    dst = a + b;
}
)

FaseAutoAddingUnivFunction(Square,
void Square(const float& in, float& dst) {
    dst = in * in;
}
)

FaseAutoAddingUnivFunction(Print,
void Print(const float& in) {
    std::cout << in << std::endl;
}
)

}

#endif // FUNCS_H_20191210
