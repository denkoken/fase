
#ifndef FASE_H_20190215
#define FASE_H_20190215

#include <shared_mutex>

#include "common.h"
#include "manager.h"
#include "univ_functions.h"
#include "variable.h"

namespace fase {

// =============================================================================
// =========================== User Interface ==================================
// =============================================================================

template <class... Parts>
class Fase : public Parts... {
public:
    Fase();

    template <typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     const std::vector<std::string>& arg_names,
                     const std::string&              description = "");
    template <typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     const std::vector<std::string>& arg_names,
                     const std::string&              description,
                     std::vector<Variable>&&         default_args);

    /**
     * @brief
     *      add functions about the type, used when save/load,
     *      generate native code.
     *
     * @tparam T
     *      User defined type
     *
     * @param name
     *      string of the type
     * @param serializer
     *      should be used when save editting project.
     * @param deserializer
     *      should be used when load editting project.
     * @param def_maker
     *      should be used when generate native code of editting project.
     *
     * @return
     *      succeeded or not (maybe this will be allways return true.)
     */
    template <typename T>
    bool registerTextIO(const std::string&                     name,
                        std::function<std::string(const T&)>&& serializer,
                        std::function<T(const std::string&)>&& deserializer,
                        std::function<std::string(const T&)>&& def_maker);

private:
    std::shared_ptr<CoreManager> pcm;
    std::shared_timed_mutex      cm_mutex;

    std::map<std::type_index, TypeStringConverters> converter_map;

    std::tuple<std::shared_lock<std::shared_timed_mutex>,
               std::shared_ptr<const CoreManager>>
    getReader(const std::chrono::nanoseconds& wait_time =
                      std::chrono::nanoseconds{-1}) override;
    std::tuple<std::unique_lock<std::shared_timed_mutex>,
               std::shared_ptr<CoreManager>>
    getWriter(const std::chrono::nanoseconds& wait_time =
                      std::chrono::nanoseconds{-1}) override;
};

#define FaseAddFunctionBuilder(func, arg_types, arg_names, ...) \
    fase::AddingUnivFuncHelper<void arg_types>::Gen(            \
            #func, FaseExpandList(arg_names), func, __VA_ARGS__);

// =============================================================================
// =========================== Non User Interface ==============================
// =============================================================================

template <bool head, bool... tails>
constexpr bool is_all_ok() {
    if constexpr (sizeof...(tails) == 0) {
        return head;
    } else {
        return head && is_all_ok<tails...>();
    }
}

template <typename... Args>
std::vector<Variable> GetDefaultValueVariables() {
    if constexpr (sizeof...(Args) == 0) {
        return {};
    } else {
        return {std::make_shared<std::decay_t<Args>>()...};
    }
}

void SetupTypeConverters(std::map<std::type_index, TypeStringConverters>*);

template <typename... Args>
std::vector<bool> GetIsInputArgs() {
    return {!(std::is_lvalue_reference_v<Args> &&
              std::is_same_v<std::remove_reference_t<Args>,
                             std::decay_t<Args>>)...};
}

template <class... Parts>
inline Fase<Parts...>::Fase()
    : Parts()..., pcm(std::make_shared<CoreManager>()) {
    auto succeeded_flags = {std::make_tuple(std::string(typeid(Parts).name()),
                                            Parts::init())...};
    for (auto [name, f] : succeeded_flags) {
        if (!f) {
            std::cerr << "Fase Parts : \"" + name + "\"'s init() is failed"
                      << std::endl;
        }
    }
    SetupTypeConverters(&converter_map);
}

template <class... Parts>
template <typename... Args>
inline bool Fase<Parts...>::addUnivFunc(
        const UnivFunc& func, const std::string& f_name,
        const std::vector<std::string>& arg_names,
        const std::string& description, std::vector<Variable>&& default_args) {
    std::vector<std::type_index> types = {typeid(Args)...};
    std::vector<bool>            is_input_args = GetIsInputArgs<Args...>();
    return pcm->addUnivFunc(func, f_name, std::move(default_args),
                            {arg_names, types, is_input_args, description});
}

template <class... Parts>
template <typename... Args>
inline bool Fase<Parts...>::addUnivFunc(
        const UnivFunc& func, const std::string& f_name,
        const std::vector<std::string>& arg_names,
        const std::string&              description) {
    static_assert(
            is_all_ok<std::is_default_constructible_v<std::decay_t<Args>>...>(),
            "Fase::addUnivFunc<Args...> : "
            "If not all Args have default constructor,"
            "do not call me WITHOUT default_args!");
    std::vector<Variable> default_args = GetDefaultValueVariables<Args...>();
    std::vector<std::type_index> types = {typeid(Args)...};
    std::vector<bool>            is_input_args = GetIsInputArgs<Args...>();
    return pcm->addUnivFunc(func, f_name, std::move(default_args),
                            {arg_names, types, is_input_args, description});
}

template <class... Parts>
template <typename T>
inline bool Fase<Parts...>::registerTextIO(
        const std::string&                     name,
        std::function<std::string(const T&)>&& serializer,
        std::function<T(const std::string&)>&& deserializer,
        std::function<std::string(const T&)>&& def_maker) {
    converter_map[typeid(T)].serializer =
            [serializer = std::move(serializer)](const Variable& v) {
                return serializer(*v.getReader<T>());
            };

    converter_map[typeid(T)].deserializer =
            [deserializer = std::move(deserializer)](Variable&          v,
                                                     const std::string& str) {
                v.create<T>(deserializer(str));
            };

    converter_map[typeid(T)].checker = [](const Variable& v) {
        return v.isSameType<T>();
    };

    converter_map[typeid(T)].name = name;

    converter_map[typeid(T)].def_maker =
            [def_maker = std::move(def_maker)](const Variable& v) {
                return def_maker(*v.getReader<T>());
            };
    return true;
}

template <class... Parts>
inline std::tuple<std::shared_lock<std::shared_timed_mutex>,
                  std::shared_ptr<const CoreManager>>
Fase<Parts...>::getReader(const std::chrono::nanoseconds& wait_time) {
    std::shared_lock<std::shared_timed_mutex> lock(cm_mutex, wait_time);
    if (lock)
        return {std::move(lock),
                std::static_pointer_cast<const CoreManager>(pcm)};
    return {};
}

template <class... Parts>
inline std::tuple<std::unique_lock<std::shared_timed_mutex>,
                  std::shared_ptr<CoreManager>>
Fase<Parts...>::getWriter(const std::chrono::nanoseconds& wait_time) {
    std::unique_lock<std::shared_timed_mutex> lock(cm_mutex, wait_time);
    if (lock) {
        return {std::move(lock), pcm};
    }
    return {};
}

#define FaseExpandListHelper(...)            \
    {                                        \
        std::initializer_list<std::string> { \
            __VA_ARGS__                      \
        }                                    \
    }
#define FaseExpandList(v) FaseExpandListHelper v

template <typename... Args>
struct AddingUnivFuncHelper {};

template <typename... Args>
struct AddingUnivFuncHelper<void(Args...)> {
    template <class FaseClass, class Callable>
    static void Gen(const std::string&              f_name,
                    const std::vector<std::string>& arg_names, Callable&& f,
                    FaseClass& app, std::string description = "") {
        auto unived =
                UnivFuncGenerator<Args...>::Gen(std::forward<Callable>(f));
        app.template addUnivFunc<Args...>(unived, f_name, arg_names,
                                          description);
    }

    template <class FaseClass, class Callable>
    static void Gen(const std::string&              f_name,
                    const std::vector<std::string>& arg_names, Callable&& f,
                    FaseClass& app, std::string description,
                    std::tuple<std::decay_t<Args>...>&& default_args) {
        auto unived =
                UnivFuncGenerator<Args...>::Gen(std::forward<Callable>(f));
        app.template addUnivFunc<Args...>(
                unived, f_name, arg_names, description,
                toVariables(std::move(default_args),
                            std::index_sequence_for<Args...>()));
    }
    template <size_t... Seq>
    static std::vector<Variable> toVariables(
            std::tuple<std::decay_t<Args>...>&& a,
            std::index_sequence<Seq...>) {
        return {std::make_shared<std::decay_t<Args>>(
                std::move(std::get<Seq>(a)))...};
    }
};

}  // namespace fase

#endif  // FASE_H_20190215
