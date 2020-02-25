
#ifndef FASE_H_20190215
#define FASE_H_20190215

#include <shared_mutex>

#include "auto_uf_adder.h"
#include "common.h"
#include "constants.h"
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

    template <typename Ret, typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     FunctionUtils&& utils);
    template <typename Ret, typename... Args>
    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     FunctionUtils&&        utils,
                     std::deque<Variable>&& default_args);

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

#define FaseAddUnivFunction(func, func_form_type, arg_names, ...)              \
    [&](auto&& f) {                                                            \
        using Ftype = std::function<func_form_type>;                           \
        using T = Ftype (*)();                                                 \
        using fase::FOGtype;                                                   \
        using FuncFormPointerType = std::add_pointer_t<func_form_type>;        \
        FOGtype     fog_t;                                                     \
        T           fog = []() -> Ftype { return func; };                      \
        std::string f_name;                                                    \
        auto        callable_type = std::make_shared<std::type_index>(         \
                typeid(FuncFormPointerType));                           \
        if constexpr (std::is_same_v<std::decay_t<decltype(f)>,                \
                                     FuncFormPointerType>) {                   \
            fog_t = FOGtype::Pure;                                             \
            f_name = #func;                                                    \
        } else if constexpr (std::is_convertible_v<std::decay_t<decltype(f)>,  \
                                                   FuncFormPointerType>) {     \
            fog_t = FOGtype::Lambda;                                           \
            f_name = "LambdaExp" + std::to_string(__LINE__);                   \
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
        fase::AddingUnivFuncHelper<func_form_type>::Gen(                       \
                f_name, FaseExpandList(arg_names), fog_t, #func_form_type,     \
                #func, callable_type, fog, __VA_ARGS__);                       \
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

template <typename... Args>
std::deque<Variable> GetDefaultValueVariables() {
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
template <typename Ret, typename... Args>
inline bool Fase<Parts...>::addUnivFunc(const UnivFunc&        func,
                                        const std::string&     f_name,
                                        FunctionUtils&&        utils,
                                        std::deque<Variable>&& default_args) {
    utils.arg_types = {typeid(std::decay_t<Args>)...};
    utils.is_input_args = {IsInputType<Args>()...};
    if constexpr (!std::is_same_v<Ret, void>) {
        default_args.emplace_back(typeid(Ret));
        utils.arg_types.emplace_back(typeid(std::decay_t<Ret>));
        utils.is_input_args.emplace_back(false);
    }
    return getAPIImpl().pcm->addUnivFunc(func, f_name, std::move(default_args),
                                         std::move(utils));
}

template <class... Parts>
template <typename Ret, typename... Args>
inline bool Fase<Parts...>::addUnivFunc(const UnivFunc&    func,
                                        const std::string& f_name,
                                        FunctionUtils&&    utils) {
    static_assert((... && std::is_default_constructible_v<std::decay_t<Args>>),
                  "Fase::addUnivFunc<Args...> : "
                  "If not all Args have default constructor,"
                  "do not call me WITHOUT default_args!");
    std::deque<Variable> default_args = GetDefaultValueVariables<Args...>();
    utils.arg_types = {typeid(std::decay_t<Args>)...};
    utils.is_input_args = {IsInputType<Args>()...};
    if constexpr (!std::is_same_v<Ret, void>) {
        default_args.emplace_back(typeid(Ret));
        utils.arg_types.emplace_back(typeid(std::decay_t<Ret>));
        utils.is_input_args.emplace_back(false);
    }
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

template <typename Ret, typename... Args>
struct AddingUnivFuncHelper<Ret(Args...)> {
    template <class FaseClass, class FuncObjGenerator>
    static void
    Gen(const std::string& f_name, const std::vector<std::string>& arg_names,
        FOGtype fog_type, const std::string& func_types_repr,
        const std::string& repr, std::shared_ptr<std::type_index> callable_type,
        FuncObjGenerator&& fog, FaseClass& app, std::string description = "") {
        auto unived = UnivFuncGenerator<Ret(Args...)>::Gen(
                std::forward<FuncObjGenerator>(fog));
        FunctionUtils utils;
        utils.arg_names = arg_names;
        utils.arg_types_repr = func_types_repr;
        utils.description = description;
        utils.type = fog_type;
        utils.repr = repr;
        utils.callable_type = callable_type;

        if constexpr (!std::is_same_v<Ret, void>) {
            utils.arg_names.push_back(kReturnValueID);
        }

        app.template addUnivFunc<Ret, Args...>(unived, f_name,
                                               std::move(utils));
    }

    template <class FaseClass, class FuncObjGenerator>
    static void
    Gen(const std::string& f_name, const std::vector<std::string>& arg_names,
        FOGtype fog_type, const std::string& func_types_repr,
        const std::string& repr, std::shared_ptr<std::type_index> callable_type,
        FuncObjGenerator&& fog, FaseClass& app, std::string description,
        std::tuple<std::decay_t<Args>...>&& default_args) {
        auto unived = UnivFuncGenerator<Ret(Args...)>::Gen(
                std::forward<FuncObjGenerator>(fog));
        FunctionUtils utils;
        utils.arg_names = arg_names;
        utils.arg_types_repr = func_types_repr;
        utils.description = description;
        utils.type = fog_type;
        utils.repr = repr;
        utils.callable_type = callable_type;

        if constexpr (!std::is_same_v<Ret, void>) {
            utils.arg_names.push_back(kReturnValueID);
        }

        app.template addUnivFunc<Ret, Args...>(
                unived, f_name, std::move(utils),
                toVariables(std::move(default_args),
                            std::index_sequence_for<Args...>()));
    }
    template <size_t... Seq>
    static std::deque<Variable>
    toVariables(std::tuple<std::decay_t<Args>...>&& a,
                std::index_sequence<Seq...>) {
        return {std::make_unique<std::decay_t<Args>>(
                std::move(std::get<Seq>(a)))...};
    }
};

} // namespace fase

#endif // FASE_H_20190215
