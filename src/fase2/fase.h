
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

    bool addUnivFunc(const UnivFunc& func, const std::string& f_name,
                     std::vector<Variable>&&         default_args,
                     const std::vector<std::string>& arg_names,
                     const std::string&              description);

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
    std::shared_mutex            cm_mutex;

    std::map<std::type_index, TypeStringConverters> converter_map;

    std::tuple<std::shared_lock<std::shared_mutex>,
               std::shared_ptr<const CoreManager>>
    getReader() override;
    std::tuple<std::unique_lock<std::shared_mutex>,
               std::shared_ptr<CoreManager>>
    getWriter() override;
};

// =============================================================================
// =========================== Non User Interface ==============================
// =============================================================================

void SetupTypeConverters(std::map<std::type_index, TypeStringConverters>*);

template <class... Parts>
inline Fase<Parts...>::Fase() : Parts()... {
    SetupTypeConverters(&converter_map);
}

template <class... Parts>
inline bool Fase<Parts...>::addUnivFunc(
        const UnivFunc& func, const std::string& f_name,
        std::vector<Variable>&&         default_args,
        const std::vector<std::string>& arg_names,
        const std::string&              description) {
    pcm->addUnivFunc(func, f_name, std::move(default_args), arg_names,
                     description);
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
inline std::tuple<std::shared_lock<std::shared_mutex>,
                  std::shared_ptr<const CoreManager>>
Fase<Parts...>::getReader() {
    return {std::shared_lock<std::shared_mutex>{cm_mutex},
            std::static_pointer_cast<const CoreManager>(pcm)};
}

template <class... Parts>
inline std::tuple<std::unique_lock<std::shared_mutex>,
                  std::shared_ptr<CoreManager>>
Fase<Parts...>::getWriter() {
    return {std::unique_lock<std::shared_mutex>{cm_mutex},
            std::static_pointer_cast<CoreManager>(pcm)};
}

}  // namespace fase

#endif  // FASE_H_20190215
