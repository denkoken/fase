
#ifndef FASE_H_20190215
#define FASE_H_20190215

#include <shared_mutex>

#include "auto_uf_adder.h"
#include "common.h"
#include "manager.h"
#include "parts_base.h"
#include "univ_functions.h"
#include "variable.h"

namespace fase {

class FaseMembers;

// =============================================================================
// =========================== User Interface ==================================
// =============================================================================

template <class... Parts>
class Fase : public Parts... {
public:
    Fase();

    template <typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     FunctionUtils&& utils);
    template <typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     FunctionUtils&&         utils,
                     std::vector<Variable>&& default_args);

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
    class APIImpl;

    Variable api_impl;

    APIImpl& getAPIImpl() {
        return *api_impl.getWriter<APIImpl>();
    }

    std::shared_ptr<PartsBase::API> getAPI() override {
        return api_impl.getWriter<APIImpl>();
    }
    std::shared_ptr<const PartsBase::API> getAPI() const override {
        return api_impl.getReader<APIImpl>();
    }

    const TSCMap& getConverterMap();
};

#define FaseAddUnivFunction(func, arg_types, arg_names, ...)                   \
    [&](auto&& f) {                                                            \
        using Ftype = std::function<void arg_types>;                           \
        using T = Ftype (*)();                                                 \
        using fase::FOGtype;                                                   \
        FOGtype     fog_t;                                                     \
        T           fog = []() -> Ftype { return func; };                      \
        std::string f_name;                                                    \
        auto        callable_type =                                            \
                std::make_shared<std::type_index>(typeid(void(*) arg_types));  \
        if constexpr (std::is_same_v<std::decay_t<decltype(f)>,                \
                                     void(*) arg_types>) {                     \
            fog_t = FOGtype::Pure;                                             \
            f_name = #func;                                                    \
        } else if constexpr (std::is_convertible_v<std::decay_t<decltype(f)>,  \
                                                   void(*) arg_types>) {       \
            fog_t = FOGtype::Lambda;                                           \
            f_name = "[[lambda]]" + std::to_string(__LINE__);                  \
        } else {                                                               \
            callable_type = std::make_shared<std::type_index>(                 \
                    typeid(std::decay_t<decltype(f)>));                        \
            fog_t = FOGtype::IndependingClass;                                 \
            f_name = #func;                                                    \
            for (char c :                                                      \
                 {'[', ']', '{', '}', '(', ')', ',', '.', ' ', ';'}) {         \
                auto p = f_name.find_first_of(c);                              \
                while (p != std::string::npos) {                               \
                    f_name.erase(p, 1);                                        \
                    p = f_name.find_first_of(c);                               \
                }                                                              \
            }                                                                  \
        }                                                                      \
        fase::AddingUnivFuncHelper<void arg_types>::Gen(                       \
                f_name, FaseExpandList(arg_names), fog_t, #arg_types, #func,   \
                callable_type, fog, __VA_ARGS__);                              \
    }(func)

// TODO Make Macro for lvalue function object.

#define FaseRegisterTextIO(app, type, serializer, deserializer, def_maker)     \
    [&] {                                                                      \
        app.template registerTextIO<type>(#type, serializer, deserializer,     \
                                          def_maker);                          \
    }();

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
        return {std::make_unique<std::decay_t<Args>>()...};
    }
}

void SetupTypeConverters(std::map<std::type_index, TypeStringConverters>*);

template <class... Parts>
class Fase<Parts...>::APIImpl : public PartsBase::API {
public:
    APIImpl() {}
    APIImpl(APIImpl& a)
        : pcm(std::make_shared<CoreManager>(*a.pcm)),
          converter_map(a.converter_map) {}
    APIImpl(const APIImpl& a)
        : pcm(std::make_shared<CoreManager>(*a.pcm)),
          converter_map(a.converter_map) {}
    APIImpl& operator=(APIImpl& a) {
        pcm = std::make_shared<CoreManager>(*a.pcm);
        converter_map = a.converter_map;
        return *this;
    }
    APIImpl& operator=(const APIImpl& a) {
        pcm = std::make_shared<CoreManager>(*a.pcm);
        converter_map = a.converter_map;
        return *this;
    }
    virtual ~APIImpl() {}

    std::shared_ptr<CoreManager>    pcm = std::make_shared<CoreManager>();
    mutable std::shared_timed_mutex cm_mutex;

    TSCMap converter_map;

    std::tuple<std::shared_lock<std::shared_timed_mutex>,
               std::shared_ptr<const CoreManager>>
    getReader(const std::chrono::nanoseconds& wait_time) const override;

    std::tuple<std::unique_lock<std::shared_timed_mutex>,
               std::shared_ptr<CoreManager>>
    getWriter(const std::chrono::nanoseconds& wait_time) override;

    const TSCMap& getConverterMap() override {
        return converter_map;
    }
};

template <typename... Args>
std::vector<bool> GetIsInputArgs() {
    return {!(std::is_lvalue_reference_v<Args> &&
              std::is_same_v<std::remove_reference_t<Args>,
                             std::decay_t<Args>>)...};
}

template <class... Parts>
inline Fase<Parts...>::Fase()
    : Parts()..., api_impl(std::make_unique<APIImpl>()) {
    auto succeeded_flags = {std::make_tuple(
            std::string(type_name(typeid(Parts))), Parts::init())...};
    for (auto [name, f] : succeeded_flags) {
        if (!f) {
            std::cerr << "Fase Parts : \"" + name + "\"'s init() is failed"
                      << std::endl;
        }
    }
    SetupTypeConverters(&getAPIImpl().converter_map);

    for (auto& adder : for_macro::FuncNodeStorer::func_builder_adders) {
        adder(getAPIImpl().pcm.get());
    }
}

template <class... Parts>
template <typename... Args>
inline bool Fase<Parts...>::addUnivFunc(const UnivFunc&         func,
                                        const std::string&      f_name,
                                        FunctionUtils&&         utils,
                                        std::vector<Variable>&& default_args) {
    utils.arg_types = {typeid(std::decay_t<Args>)...};
    utils.is_input_args = GetIsInputArgs<Args...>();
    return getAPIImpl().pcm->addUnivFunc(func, f_name, std::move(default_args),
                                         std::move(utils));
}

template <class... Parts>
template <typename... Args>
inline bool Fase<Parts...>::addUnivFunc(const UnivFunc&    func,
                                        const std::string& f_name,
                                        FunctionUtils&&    utils) {
    static_assert(
            is_all_ok<std::is_default_constructible_v<std::decay_t<Args>>...>(),
            "Fase::addUnivFunc<Args...> : "
            "If not all Args have default constructor,"
            "do not call me WITHOUT default_args!");
    std::vector<Variable> default_args = GetDefaultValueVariables<Args...>();
    utils.arg_types = {typeid(std::decay_t<Args>)...};
    utils.is_input_args = GetIsInputArgs<Args...>();
    return getAPIImpl().pcm->addUnivFunc(func, f_name, std::move(default_args),
                                         std::move(utils));
}

template <class... Parts>
template <typename T>
inline bool Fase<Parts...>::registerTextIO(
        const std::string&                     name,
        std::function<std::string(const T&)>&& serializer,
        std::function<T(const std::string&)>&& deserializer,
        std::function<std::string(const T&)>&& def_maker) {
    getAPIImpl().converter_map[typeid(T)].serializer =
            [serializer = std::move(serializer)](const Variable& v) {
                return serializer(*v.getReader<T>());
            };

    getAPIImpl().converter_map[typeid(T)].deserializer =
            [deserializer = std::move(deserializer)](Variable&          v,
                                                     const std::string& str) {
                v.create<T>(deserializer(str));
            };

    getAPIImpl().converter_map[typeid(T)].checker = [](const Variable& v) {
        return v.isSameType<T>();
    };

    getAPIImpl().converter_map[typeid(T)].name = name;

    getAPIImpl().converter_map[typeid(T)].def_maker =
            [def_maker = std::move(def_maker)](const Variable& v) {
                return def_maker(*v.getReader<T>());
            };
    return true;
}

template <class... Parts>
inline std::tuple<std::shared_lock<std::shared_timed_mutex>,
                  std::shared_ptr<const CoreManager>>
Fase<Parts...>::APIImpl::getReader(
        const std::chrono::nanoseconds& wait_time) const {
    std::shared_lock<std::shared_timed_mutex> lock(cm_mutex, wait_time);
    if (lock)
        return {std::move(lock),
                std::static_pointer_cast<const CoreManager>(pcm)};
    return {};
}

template <class... Parts>
inline std::tuple<std::unique_lock<std::shared_timed_mutex>,
                  std::shared_ptr<CoreManager>>
Fase<Parts...>::APIImpl::getWriter(const std::chrono::nanoseconds& wait_time) {
    std::unique_lock<std::shared_timed_mutex> lock(cm_mutex, wait_time);
    if (lock) {
        return {std::move(lock), pcm};
    }
    return {};
}

#define FaseExpandListHelper(...)                                              \
    {                                                                          \
        std::initializer_list<std::string> {                                   \
            __VA_ARGS__                                                        \
        }                                                                      \
    }
#define FaseExpandList(v) FaseExpandListHelper v

template <typename... Args>
struct AddingUnivFuncHelper {};

template <typename... Args>
struct AddingUnivFuncHelper<void(Args...)> {
    template <class FaseClass, class FuncObjGenerator>
    static void
    Gen(const std::string& f_name, const std::vector<std::string>& arg_names,
        FOGtype fog_type, const std::string& arg_types_repr,
        const std::string& repr, std::shared_ptr<std::type_index> callable_type,
        FuncObjGenerator&& fog, FaseClass& app, std::string description = "") {
        auto unived = UnivFuncGenerator<Args...>::Gen(
                std::forward<FuncObjGenerator>(fog));
        FunctionUtils utils;
        utils.arg_names = arg_names;
        utils.arg_types_repr = arg_types_repr;
        utils.description = description;
        utils.type = fog_type;
        utils.repr = repr;
        utils.callable_type = callable_type;
        app.template addUnivFunc<Args...>(unived, f_name, std::move(utils));
    }

    template <class FaseClass, class FuncObjGenerator>
    static void
    Gen(const std::string& f_name, const std::vector<std::string>& arg_names,
        FOGtype fog_type, const std::string& arg_types_repr,
        const std::string& repr, std::shared_ptr<std::type_index> callable_type,
        FuncObjGenerator&& fog, FaseClass& app, std::string description,
        std::tuple<std::decay_t<Args>...>&& default_args) {
        auto unived = UnivFuncGenerator<Args...>::Gen(
                std::forward<FuncObjGenerator>(fog));
        FunctionUtils utils;
        utils.arg_names = arg_names;
        utils.arg_types_repr = arg_types_repr;
        utils.description = description;
        utils.type = fog_type;
        utils.repr = repr;
        utils.callable_type = callable_type;
        app.template addUnivFunc<Args...>(
                unived, f_name, std::move(utils),
                toVariables(std::move(default_args),
                            std::index_sequence_for<Args...>()));
    }
    template <size_t... Seq>
    static std::vector<Variable>
    toVariables(std::tuple<std::decay_t<Args>...>&& a,
                std::index_sequence<Seq...>) {
        return {std::make_unique<std::decay_t<Args>>(
                std::move(std::get<Seq>(a)))...};
    }
};

} // namespace fase

#endif // FASE_H_20190215
