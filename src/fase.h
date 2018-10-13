#ifndef FASE_H_20180617
#define FASE_H_20180617

#define NOMINMAX

#include "core.h"
#include "core_util.h"
#include "editor.h"
#include "type_utils.h"

namespace fase {

template <class... Parts>
class Fase : public Parts... {
public:
    Fase();

    template <typename Ret, typename... Args>
    bool addFunctionBuilder(
            const std::string& func_repr,
            const std::function<Ret(Args...)>& func_val,
            const std::array<std::string, sizeof...(Args)>& arg_type_reprs,
            const std::array<std::string, sizeof...(Args)>& default_arg_reprs,
            const std::array<std::string, sizeof...(Args)>& arg_names = {},
            const std::array<Variable, sizeof...(Args)>& default_args = {}) {
        // Register to the core system
        return core->template addFunctionBuilder<Ret, Args...>(
                func_repr, func_val, arg_type_reprs, default_arg_reprs,
                arg_names, default_args);
    }

    template <typename T>
    bool registerTextIO(const std::string& name,
                        std::function<std::string(const T&)>&& serializer,
                        std::function<T(const std::string&)>&& deserializer,
                        std::function<std::string(const T&)>&& def_maker);

private:
    std::shared_ptr<FaseCore> getCore() {
        return core;
    }

    std::shared_ptr<FaseCore> core;

    TypeUtils type_utils;
};

}  // namespace fase

#include "fase_impl.h"

#endif  // FASE_H_20180617
