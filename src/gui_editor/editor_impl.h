#ifndef EDITOR_IMPL_H_20181013
#define EDITOR_IMPL_H_20181013

#include "../editor.h"

#include <functional>
#include <list>
#include <memory>
#include <vector>

namespace fase {
namespace guieditor {

template <typename T>
bool GUIEditor::addVarEditor(VarEditor<T>&& var_editor) {
    auto pfunc = std::make_shared<VarEditor<T>>(
            std::forward<VarEditor<T>>(var_editor));

    auto var_editor_wraped = [pfunc](const char* label, const Variable& v) {
        auto p = (*pfunc)(label, *v.getReader<T>());
        if (p) return Variable(std::move(*p));
        return Variable();
    };

    // TODO fix for T is non copyable.
#define MakeContainerEditor(Container)                         \
    [pfunc](const char* label, const Variable& vec_v) {        \
        const auto& vec = *vec_v.getReader<Container<T>>();    \
        std::unique_ptr<Container<T>> dst;                     \
        long i = 0;                                            \
        for (const auto& v : vec) {                            \
            auto p = (*pfunc)(label, v);                       \
            if (p) {                                           \
                if (!dst) {                                    \
                    dst = std::make_unique<Container<T>>(vec); \
                }                                              \
                *(std::begin(*dst) + i) = std::move(*p);       \
            }                                                  \
            i++;                                               \
        }                                                      \
        if (dst) return Variable(std::move(*dst));             \
        return Variable();                                     \
    };

    auto vector_var_editor_wraped = MakeContainerEditor(std::vector);
    // list iterator don't have operator +,
    // auto list_var_editor_wraped = MakeContainerEditor(std::list);

#undef MakeContainerEditor

    addVarEditor(&typeid(T), std::move(var_editor_wraped));
    addVarEditor(&typeid(std::vector<T>), std::move(vector_var_editor_wraped));
    // addVarEditor(&typeid(std::list<T>), std::move(list_var_editor_wraped));

    return true;
}

}  // namespace guieditor
}  // namespace fase

#endif  // EDITOR_IMPL_H_20181013
