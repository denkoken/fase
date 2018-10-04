
#ifndef CONSTEXPR_STRING_H_20180926
#define CONSTEXPR_STRING_H_20180926

#include <array>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>

namespace fase {

template <typename Type>
struct Clean {
    using type = typename std::remove_reference<
            typename std::remove_const<Type>::type>::type;
};

template <typename T, typename... Args>
constexpr size_t ArgC(T (*)(Args...)) {
    return sizeof...(Args);
}

template <bool V>
constexpr auto getTruthValue();

template <>
constexpr auto getTruthValue<true>() {
    return std::true_type{};
}

template <>
constexpr auto getTruthValue<false>() {
    return std::false_type{};
}

template <typename... Types>
struct TypeSequence {
    constexpr static size_t size = sizeof...(Types);
    template <size_t N>
    using at = typename Clean<decltype(
            std::get<N>(std::declval<std::tuple<Types...>>()))>::type;
    using tuple = std::tuple<Types...>;
    constexpr TypeSequence() {}
};

template <typename... Types>
constexpr auto make_type_sequence(Types...) {
    return TypeSequence<Types...>{};
}

template <typename... Types>
constexpr auto FromTuple(std::tuple<Types...>) {
    return TypeSequence<Types...>{};
}

template <typename... TypeSequences>
constexpr auto joint() {
    return FromTuple(std::tuple_cat(typename TypeSequences::tuple{}...));
}

template <typename... TypeSequences>
constexpr auto joint(TypeSequences...) {
    return joint<TypeSequences...>();
}

template <>
constexpr auto joint<>() {
    return TypeSequence<>();
}

// constexpr Array
template <typename T, size_t N>
class Array {
public:
    constexpr T& operator[](size_t idx) {
        return array[idx];
    }
    constexpr const T& operator[](size_t idx) const {
        return array[idx];
    }

    constexpr size_t size() const {
        return N;
    }

    const T* begin() const {
        return array;
    }
    T* begin() {
        return array;
    }

    const T* end() const {
        return array + N;
    }
    T* end() {
        return array + N;
    }

    T array[N];
};

constexpr size_t npos = std::numeric_limits<size_t>::max();

template <size_t Size>
class String {
public:
    constexpr char& operator[](size_t idx) {
        return char_array[idx];
    }
    constexpr const char& operator[](size_t idx) const {
        return char_array[idx];
    }

    constexpr size_t size() const {
        for (size_t i = 0; i < Size; i++) {
            if (char_array[i] == '\0') {
                return i;
            }
        }
        return Size;
    }

    constexpr size_t find(char c, size_t start = 0, size_t end = npos) const {
        if (end == npos) {
            end = size();
        }
        for (size_t i = start; i < end; i++) {
            if (char_array[i] == c) {
                return i;
            }
        }
        return npos;
    }

    template <size_t N>
    constexpr size_t find(String<N> str, size_t start = 0,
                          size_t end = npos) const {
        if (end == npos) {
            end = size();
        }
        size_t counter = 0;
        for (size_t i = start; i < end; i++) {
            if (char_array[i] == str.char_array[counter]) {
                counter += 1;
                if (str.char_array[counter + 1] == '\0') {
                    return i - counter + 1;
                }
            } else {
                counter = 0;
            }
        }
        return npos;
    }

    template <size_t N>
    constexpr bool isFound(String<N> str, size_t start = 0,
                           size_t end = npos) const {
        if (end == npos) {
            end = size();
        }
        size_t counter = 0;
        for (size_t i = start; i < end; i++) {
            if (char_array[i] == str.char_array[counter]) {
                if (str.char_array[counter + 1] == '\0') {
                    return true;
                }
                counter += 1;
            } else {
                counter = 0;
            }
        }
        return false;
    }

    constexpr size_t count(char c, size_t start = 0, size_t end = Size) const {
        if (end >= Size) {
            end = size() - 1;
        }
        size_t count = 0;
        for (size_t i = start; i <= end; i++) {
            if (char_array[i] == c) {
                count += 1;
            }
        }
        return count;
    }

    std::string toStd(size_t start = 0, size_t end = Size) {
        return std::string(char_array + start, char_array + end - 1);
    }

    const char* get() const {
        return char_array;
    }

    char char_array[Size];
};

template <size_t N, size_t M>
constexpr size_t getArgsEnd(String<N> f_name, String<M> code) {
    size_t f_name_pos = code.find(f_name);
    bool f_end = false;
    int r_blacket_c = 0;
    for (size_t i = 0 + f_name_pos; i < M; i++) {
        char c = code[i];
        if (c == '(') {
            r_blacket_c += 1;
            f_end = true;
        } else if (c == ')') {
            r_blacket_c -= 1;
        }

        if (f_end && !r_blacket_c) {
            return i;
        }
    }
    return M;
}

template <size_t N, size_t M>
constexpr Array<size_t, N> getArgStarts(size_t start, String<M> code) {
    Array<size_t, N> dst{};

    dst[0] = start;
    for (size_t i = 1; i < N; i++) {
        size_t a_bracket_c = 0;
        size_t r_bracket_c = 0;
        size_t brace_c = 0;
        for (size_t p = start; p < M; p++) {
            if (code[p] == '<') {
                a_bracket_c++;
            } else if (code[p] == '>') {
                a_bracket_c--;
            } else if (code[p] == '(') {
                r_bracket_c++;
            } else if (code[p] == ')') {
                r_bracket_c--;
            } else if (code[p] == '{') {
                brace_c++;
            } else if (code[p] == '}') {
                brace_c--;
            }
            if (a_bracket_c == 0 && r_bracket_c == 0 && brace_c == 0 &&
                code[p] == ',') {
                dst[i] = p + 1;
                break;
            }
        }
        start = dst[i];
    }
    return dst;
}

template <size_t N, size_t M>
constexpr Array<size_t, N> getArgEnds(size_t start, size_t end,
                                      String<M> code) {
    Array<size_t, N> dst{};

    for (size_t i = 0; i < N - 1; i++) {
        size_t a_bracket_c = 0;
        size_t r_bracket_c = 0;
        size_t brace_c = 0;
        for (size_t p = start + 1; p < end; p++) {
            if (code[p] == '<') {
                a_bracket_c++;
            } else if (code[p] == '>') {
                a_bracket_c--;
            } else if (code[p] == '(') {
                r_bracket_c++;
            } else if (code[p] == ')') {
                r_bracket_c--;
            } else if (code[p] == '{') {
                brace_c++;
            } else if (code[p] == '}') {
                brace_c--;
            }
            if (a_bracket_c == 0 && r_bracket_c == 0 && brace_c == 0 &&
                code[p] == ',') {
                dst[i] = p;
                break;
            }
        }
        start = dst[i];
    }
    dst[N - 1] = end;
    return dst;
}

template <size_t N, size_t M>
constexpr Array<bool, N> IsDefaultValues(Array<size_t, N> arg_starts,
                                         Array<size_t, N> arg_ends,
                                         String<M> code) {
    Array<bool, N> dst{};
    for (size_t i = 0; i < N; i++) {
        dst[i] = code.count('=', arg_starts[i], arg_ends[i]);
    }
    return dst;
}

template <size_t N, size_t M>
constexpr Array<size_t, N> getDefaultVArgC(Array<size_t, N> arg_starts,
                                           Array<size_t, N> arg_ends,
                                           String<M> code) {
    Array<size_t, N> dst{};

    for (size_t i = 0; i < N; i++) {
        if (!code.count('=', arg_starts[i], arg_ends[i])) {
            continue;
        }

        size_t dv_s = code.find('=', arg_starts[i]);

        dst[i] = 1;
        size_t a_bracket_c = 0;
        bool f = false;
        for (size_t p = dv_s; p < arg_ends[i]; p++) {
            if (code[p] == '<') {
                a_bracket_c++;
            } else if (code[p] == '>') {
                a_bracket_c--;
            }
            if (a_bracket_c == 0 && (code[p] == '(' || code[p] == '{')) {
                f = true;
            } else if (f && (code[p] == ')' || code[p] == '}')) {
                dst[i] = 0;
                break;
            } else {
                f = false;
            }
            if (a_bracket_c == 0 && code[p] == ',') {
                dst[i]++;
            }
        }
    }

    return dst;
}

template <size_t M>
constexpr size_t SearchComma(String<M> code_str, size_t dv_s, size_t end,
                             size_t count) {
    size_t a_bracket_c = 0;
    size_t i = 0;
    for (size_t p = dv_s; p < end; p++) {
        if (code_str[p] == '<') {
            a_bracket_c++;
        } else if (code_str[p] == '>') {
            a_bracket_c--;
        }
        if (a_bracket_c == 0 && code_str[p] == ',') {
            if (i == count) {
                return p;
            }
            i++;
        }
    }
    return dv_s;
};

inline std::vector<std::string> split(const std::string& str, const char& sp) {
    size_t start = 0;
    size_t end;
    std::vector<std::string> dst;
    while (true) {
        end = str.find_first_of(sp, start);
        dst.emplace_back(std::string(str, start, end - start));
        start = end + 1;
        if (end >= str.size()) {
            break;
        }
    }
    if (dst.empty()) {
        return {str};
    }
    return dst;
}

inline std::string CleanStr(std::string str) {
    while (str.find(' ') == 0) {
        str = str.substr(1, str.size());
    }
    if (str.find('&') == 0) {
        str = str.substr(1, str.size());
    }
    while (str.find('*') == 0) {
        str = str.substr(1, str.size());
    }
    while (str.rfind(' ') == str.size() - 1) {
        str = str.substr(0, str.size() - 1);
    }
    return str;
}

namespace for_macro {

template <typename T>
inline bool Delete(T) {
    return false;
}

template <>
inline bool Delete(char* ch_a) {
    delete[] ch_a;
    return true;
}

template <typename Type, typename... Args>
inline void CBracketMake(Variable* v, Args... args) {
    *v = Type(args...);
    std::vector<bool> dummy = {Delete(args)...};
}

template <typename Type, typename... Args>
inline void BraceMake(Variable* v, Args... args) {
    *v = Type{args...};
    std::vector<bool> dummy = {Delete(args)...};
}

template <typename Type>
inline Type FromString(const std::string& str);

template <>
inline double FromString<double>(const std::string& str) {
    try {
        return std::stod(str);
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0.0;
    }
}

template <>
inline int FromString<int>(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0;
    }
}

template <>
inline float FromString<float>(const std::string& str) {
    try {
        return std::stof(str);
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0;
    }
}

template <>
inline unsigned long long FromString<unsigned long long>(
        const std::string& str) {
    if (std::string::npos != str.find("nullptr")) {
        return 0;
    }
    try {
        return std::stoull(str);
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0;
    }
}

template <>
inline unsigned long FromString<unsigned long>(const std::string& str) {
    if (std::string::npos != str.find("nullptr")) {
        return 0;
    }
    try {
        return std::stoul(str);
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0;
    }
}

template <>
inline unsigned FromString<unsigned>(const std::string& str) {
    if (std::string::npos != str.find("nullptr")) {
        return 0;
    }
    try {
        return unsigned(std::stoi(str));
    } catch (...) {
        std::cerr << "FromString err : " << str << std::endl;
        return 0;
    }
}

template <>
inline std::string FromString<std::string>(const std::string& str) {
    std::string str_ = str;
    while (str_.find(' ') == 0) {
        str_ = str_.substr(1, str_.size());
    }
    while (str_.rfind(' ') == str_.size() - 1) {
        str_ = str_.substr(0, str_.size() - 1);
    }
    if (str_.find('"') == 0) {
        str_.substr(1, str_.size());
    }
    if (str_.rfind("\"s") == str_.size() - 2) {
        str_.substr(0, str_.size() - 2);
    }
    if (str_.rfind('"') == str_.size() - 1) {
        str_ = str_.substr(0, str_.size() - 1);
    }
    return str_;
}

template <>
inline char* FromString<char*>(const std::string& str) {
    std::string str_ = str;
    while (str_.find(' ') == 0) {
        str_ = std::string(std::begin(str_) + 1, std::end(str_));
    }
    while (str_.rfind(' ') == str_.size() - 1) {
        str_ = std::string(std::begin(str_), std::end(str_) - 1);
    }
    if (str_.find('"') == 0) {
        str_ = std::string(std::begin(str_) + 1, std::end(str_));
    }
    if (str_.rfind('"') == str_.size() - 1) {
        str_ = std::string(std::begin(str_), std::end(str_) - 1);
    }
    if (str_.rfind("\"s") == str_.size() - 2) {
        str_ = std::string(std::begin(str_), std::end(str_) - 2);
    }

    // check fase::for_macro::Delete.
    char* ch_a = new char[str_.size() + 1]; // contain '\0'.

    for (size_t i = 0; i <= str_.size(); i++) {
        ch_a[i] = str_[i];
    }
    return ch_a;
}

inline std::vector<std::string> GetArgStrs(std::string args_str) {

    while (args_str.find(' ') == 0 || args_str.find('{') == 0 ||
           args_str.find('(') == 0) {
        args_str = std::string(std::begin(args_str) + 1, std::end(args_str));
    }
    while (args_str.rfind(' ') == args_str.size() - 1 ||
           args_str.rfind('}') == args_str.size() - 1 ||
           args_str.rfind(')') == args_str.size() - 1) {
        args_str = std::string(std::begin(args_str), std::end(args_str) - 1);
    }
    std::vector<std::string> dst;
    std::vector<std::stringstream> buf;

    buf.push_back(std::stringstream{});

    int c_blacket_c = 0;

    for (auto i = std::begin(args_str); i != std::end(args_str); i++) {
        char c = *i;
        if (c == '<') {
            c_blacket_c += 1;
        } else if (c == '>') {
            c_blacket_c -= 1;
        }

        if (c == ',' && c_blacket_c == 0) {
            dst.push_back(buf.back().str());
            buf.push_back(std::stringstream{});
        } else {
            buf.back() << c;
        }
    }
    dst.push_back(buf.back().str());

    return dst;
}

template <typename Type, class ExistingDefault, class IsBrace, class InfoType,
          size_t... Seq>
inline bool GenVariableFromString(const std::string& code, Variable* v,
                                  TypeSequence<Type>, TypeSequence<InfoType>,
                                  std::index_sequence<Seq...>, ExistingDefault,
                                  IsBrace) {
    if constexpr (ExistingDefault::value) {
        if constexpr (InfoType::size == 0) {
            // Empty Init
            BraceMake<Type>(v);
            return true;
        }
        std::vector<std::string> strs = GetArgStrs(code);
        for (auto& str : strs) {
            if (str.empty()) {
                return false;
            }
        }

        auto args = std::make_tuple(
                FromString<typename InfoType::template at<Seq>>(strs[Seq])...);
        if constexpr (IsBrace::value) {
            BraceMake<Type, typename InfoType::template at<Seq>...>(
                    v, std::get<Seq>(args)...);
        } else {
            CBracketMake<Type, typename InfoType::template at<Seq>...>(
                    v, std::get<Seq>(args)...);
        }
        return true;
    }
    return false;
}

inline std::string getArgsStr(const std::string& f_name, std::string code) {
    size_t f_name_pos = code.find(f_name);
    std::string dst;
    if (f_name_pos != npos) {
        dst = std::string(std::begin(code) + long(f_name_pos) + 1,
                          std::end(code));
    } else {
        dst = std::string(std::begin(code), std::end(code));
    }
    return dst;
}

template <size_t M>
constexpr inline bool CheckIsBrace(String<M> code, size_t start, size_t end) {
    size_t dv_s = code.find('=', start, end);
    if (dv_s == npos) {
        return false;
    }

    return bool(code.count('{', dv_s, end)) && bool(code.count('}', dv_s, end));
}

}  // namespace for_macro

class FuncNodeStorer {
public:
    template <typename Ret, typename... Args>
    FuncNodeStorer(std::string func_name, const std::string& code,
                   Ret (*fp)(Args...)) {
        if (codes.count(func_name)) {
            return;
        }
        codes[func_name] = code;
        f_buf = [fp, func_name](
                        auto* app,
                        const std::vector<std::string>& arg_type_reprs,
                        const std::vector<std::string>& default_arg_reprs,
                        const std::vector<std::string>& arg_names,
                        const std::vector<Variable>& default_args) {
            constexpr size_t N = sizeof...(Args);
            std::array<std::string, N> arg_type_reprs_;
            std::array<std::string, N> default_arg_reprs_;
            std::array<std::string, N> arg_names_;
            std::array<Variable, N> default_args_;

            assert(arg_type_reprs.size() == N);
            assert(default_arg_reprs.size() == N);
            assert(arg_names.size() == N);
            assert(default_args.size() == N);

            std::copy_n(arg_type_reprs.begin(), N, arg_type_reprs_.begin());
            std::copy_n(arg_names.begin(), N, arg_names_.begin());
            std::copy_n(default_arg_reprs.begin(), N,
                        default_arg_reprs_.begin());
            std::copy_n(default_args.begin(), N, default_args_.begin());

            app->addFunctionBuilder(func_name, std::function<Ret(Args...)>(fp),
                                    arg_type_reprs_, default_arg_reprs_,
                                    arg_names_, default_args_);
        };
    }

    template <size_t N, class ArgInfoTuples, class ArgTypesTuple,
              class ExistingDefaults, class IsBraces, size_t... Seq>
    void build(const std::string& code, const Array<size_t, N>& begins,
               const Array<size_t, N>& ends) {
        std::vector<std::string> arg_type_reprs;
        std::vector<std::string> default_arg_reprs;
        std::vector<std::string> arg_names;

        std::vector<std::string> arg_code;
        for (size_t i = 0; i < N; i++) {
            arg_code.push_back(code.substr(begins[i], ends[i] - begins[i]));
        }

        convert(arg_code, &arg_type_reprs, &default_arg_reprs, &arg_names);

        constexpr Array<size_t, N> default_v_arg_c = {
                {ArgInfoTuples::template at<Seq>::size...}};

#ifndef NDEBUG
        for (auto& str : arg_names) {
            std::clog << str << "|";
        }
        std::clog << std::endl;

        for (auto& str : arg_type_reprs) {
            std::clog << str << "|";
        }
        std::clog << std::endl;
        for (auto& str : default_arg_reprs) {
            std::clog << str << "|";
        }
        std::clog << std::endl;
        for (auto& i : default_v_arg_c) {
            std::clog << i << " ";
        }
        std::clog << std::endl;
        std::array<bool, N> is_braces = {
                {IsBraces::template at<Seq>::value...}};
        for (size_t i = 0; i < N; i++) {
            std::clog << is_braces[i] << " ";
        }
        std::clog << std::endl;
#endif

        std::vector<Variable> default_args(N);

        std::array<std::string, N> default_v_arg_strs = {{for_macro::getArgsStr(
                arg_type_reprs[Seq], default_arg_reprs[Seq])...}};

        std::vector<bool> success = {{for_macro::GenVariableFromString(
                default_v_arg_strs[Seq], &default_args[Seq],
                TypeSequence<typename ArgTypesTuple::template at<Seq>>{},
                TypeSequence<typename ArgInfoTuples::template at<Seq>>{},
                std::make_index_sequence<default_v_arg_c[Seq]>(),
                typename ExistingDefaults::template at<Seq>(),
                typename IsBraces::template at<Seq>())...}};

        // TODO init default_args
        func_builder_adders.push_back([f = this->f_buf, arg_names, default_args,
                                       default_arg_reprs,
                                       arg_type_reprs](auto* app) {
            f(app, arg_type_reprs, default_arg_reprs, arg_names, default_args);
        });
    }

    static inline std::vector<std::function<void(Fase<GUIEditor>*)>>
            func_builder_adders;
    static inline std::map<std::string, std::string> codes;

private:
    void convert(const std::vector<std::string>& arg_strs,
                 std::vector<std::string>* arg_type_reprs,
                 std::vector<std::string>* default_arg_reprs,
                 std::vector<std::string>* arg_names) {
        for (const auto& arg_str : arg_strs) {
            // Find argument names
            std::vector<std::string> splited =
                    split(split(arg_str, '=')[0], ' ');
            for (auto i = splited.end() - 1; i != splited.begin(); i--) {
                if (i->empty()) {
                    continue;
                }
                arg_names->push_back(CleanStr(*i));
                break;
            }

            // Find argument types
            const std::string& name = arg_names->back();

            long name_pos = long(split(arg_str, '=')[0].rfind(name));

            std::string type_str(std::begin(arg_str),
                                 std::begin(arg_str) + name_pos);

            type_str = CleanStr(type_str);

            arg_type_reprs->emplace_back(std::move(type_str));

            if (arg_str.find("=") != std::string::npos) {
                size_t pos = arg_str.find("=") + 1;
                auto default_value_str =
                        arg_str.substr(pos, arg_str.size() - 1);
                default_value_str = CleanStr(default_value_str);

                default_arg_reprs->emplace_back(std::move(default_value_str));
            } else {
                default_arg_reprs->emplace_back(std::string{});
            }
        }
    }

    std::function<void(Fase<GUIEditor>*, const std::vector<std::string>&,
                       const std::vector<std::string>&,
                       const std::vector<std::string>&,
                       const std::vector<Variable>&)>
            f_buf;
};

#define Fase_StrFound(code_str, start, end, var) \
    code_str.isFound(fase::String<sizeof(var)>{var}, start, end)
#define Fase_SpecifyTypeStr(code_str, start, end, var)                    \
    else if constexpr (code_str.isFound(fase::String<sizeof(#var)>{#var}, \
                                        start, end)) {                    \
        return fase::TypeSequence<var>();                                 \
    }

#define Fase_CE constexpr static inline

#define FaseAutoAddingFunctionBuilder_B(func_name, code, c)                    \
    code template <size_t... Seq>                                              \
    class AutoFunctionBuilderAdder_##func_name##c {                            \
    public:                                                                    \
        template <typename Ret, typename... Args>                              \
        AutoFunctionBuilderAdder_##func_name##c(Ret (*fp)(Args...)) {          \
            using namespace fase;                                              \
            fase::FuncNodeStorer a(#func_name, #code, fp);                     \
            a.build<N, decltype(infos),                                        \
                    TypeSequence<typename Clean<Args>::type...>,               \
                    decltype(existing_default_vs), decltype(is_brace),         \
                    Seq...>(#code, arg_start_poss, arg_end_poss);              \
        }                                                                      \
                                                                               \
        template <size_t N>                                                    \
        using Str = fase::String<N>;                                           \
        template <typename T, size_t N>                                        \
        using Arr = fase::Array<T, N>;                                         \
                                                                               \
        Fase_CE size_t C_L = sizeof(#code) + 10;                               \
        Fase_CE size_t N = fase::ArgC(func_name);                              \
        Fase_CE Str<C_L> code_str{#code};                                      \
        Fase_CE Str<C_L> func_str{#func_name};                                 \
        Fase_CE size_t start = code_str.find(func_str) + func_str.size() + 1;  \
        Fase_CE size_t end = fase::getArgsEnd(func_str, code_str);             \
        Fase_CE Arr<size_t, N> arg_start_poss =                                \
                fase::getArgStarts<N, C_L>(start, code_str);                   \
        Fase_CE Arr<size_t, N> arg_end_poss =                                  \
                fase::getArgEnds<N, C_L>(start, end, code_str);                \
        Fase_CE Arr<size_t, N> default_v_arg_c =                               \
                fase::getDefaultVArgC(arg_start_poss, arg_end_poss, code_str); \
        Fase_CE auto existing_default_vs = fase::make_type_sequence(           \
                fase::getTruthValue<bool(code_str.count(                       \
                        '=', arg_start_poss[Seq], arg_end_poss[Seq]))>()...);  \
        Fase_CE auto is_brace = fase::make_type_sequence(                      \
                fase::getTruthValue<fase::for_macro::CheckIsBrace(             \
                        code_str, arg_start_poss[Seq],                         \
                        arg_end_poss[Seq])>()...);                             \
                                                                               \
        template <size_t Start, size_t End>                                    \
        constexpr static auto specifyType() {                                  \
            /* other type */                                                   \
            if constexpr (code_str.find('"', Start, End) != fase::npos &&      \
                          code_str.find('"', Start, End) <                     \
                                  code_str.find(Str<3>{"\"s"}, Start, End)) {  \
                return fase::TypeSequence<std::string>();                      \
            } else if constexpr (Fase_StrFound(code_str, Start, End, "\"")) {  \
                return fase::TypeSequence<char*>();                            \
            }                                                                  \
            /* cast type */                                                    \
            Fase_SpecifyTypeStr(code_str, Start, End, std::string)            \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned char)          \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned short)         \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned int)           \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned long long)     \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned long)          \
            Fase_SpecifyTypeStr(code_str, Start, End, unsigned)               \
            Fase_SpecifyTypeStr(code_str, Start, End, float)                  \
            Fase_SpecifyTypeStr(code_str, Start, End, double)                 \
            Fase_SpecifyTypeStr(code_str, Start, End, long double)            \
            Fase_SpecifyTypeStr(code_str, Start, End, short)                  \
            Fase_SpecifyTypeStr(code_str, Start, End, char)                   \
            Fase_SpecifyTypeStr(code_str, Start, End, int)                    \
            Fase_SpecifyTypeStr(code_str, Start, End, long)                   \
            Fase_SpecifyTypeStr(code_str, Start, End, size_t)                 \
            /* literal type */                                                 \
            else if constexpr (Fase_StrFound(code_str, Start, End, "ull")) {   \
                return fase::TypeSequence<unsigned long long>();               \
            }                                                                  \
            else if constexpr (Fase_StrFound(code_str, Start, End, "ul")) {    \
                return fase::TypeSequence<unsigned long>();                    \
            }                                                                  \
            else if constexpr (Fase_StrFound(code_str, Start, End, "l")) {     \
                return fase::TypeSequence<long>();                             \
            }                                                                  \
            else if constexpr (Fase_StrFound(code_str, Start, End, ".")) {     \
                return fase::TypeSequence<double>();                           \
            }                                                                  \
            else if constexpr (Fase_StrFound(code_str, Start, End, "f")) {     \
                return fase::TypeSequence<float>();                            \
            }                                                                  \
            else if constexpr (Fase_StrFound(code_str, Start, End,             \
                                             "nullptr")) {                     \
                return fase::TypeSequence<std::nullptr_t>();                   \
            }                                                                  \
            else {                                                             \
                return fase::TypeSequence<int>();                              \
            }                                                                  \
        }                                                                      \
                                                                               \
        template <size_t Count, size_t ArgN>                                   \
        constexpr static auto MakeNSizeTuple() {                               \
            constexpr size_t dv_s = code_str.find('=', arg_start_poss[ArgN]);  \
            constexpr size_t dv_arg_start =                                    \
                    ((Count == 0) ?                                            \
                             dv_s :                                            \
                             fase::SearchComma(code_str, dv_s,                 \
                                               arg_end_poss[ArgN], Count));    \
            constexpr size_t dv_arg_end =                                      \
                    ((Count == default_v_arg_c[ArgN] - 1) ?                    \
                             arg_end_poss[ArgN] :                              \
                             fase::SearchComma(code_str, dv_s,                 \
                                               arg_end_poss[ArgN],             \
                                               Count + 1));                    \
                                                                               \
            constexpr auto type = specifyType<dv_arg_start, dv_arg_end>();     \
            if constexpr (0 == default_v_arg_c[ArgN]) {                        \
                return fase::TypeSequence<>{};                                 \
            } else if constexpr (Count == default_v_arg_c[ArgN] - 1) {         \
                return type;                                                   \
            } else {                                                           \
                return fase::joint(type, MakeNSizeTuple<Count + 1, ArgN>());   \
            }                                                                  \
        }                                                                      \
        Fase_CE auto infos =                                                   \
                fase::make_type_sequence(MakeNSizeTuple<0, Seq>()...);         \
    };                                                                         \
    template <size_t... Seq>                                                   \
    static auto make_AutoFunctionBuilderAdder_##func_name##c(                  \
            std::index_sequence<Seq...>) {                                     \
        return AutoFunctionBuilderAdder_##func_name##c<Seq...>(func_name);     \
    }                                                                          \
    static auto test##func_name##c =                                           \
            make_AutoFunctionBuilderAdder_##func_name##c(                      \
                    std::make_index_sequence<fase::ArgC(func_name)>());

#ifdef __COUNTER__
#define FaseAutoAddingFunctionBuilder_(func_name, code, c) \
    FaseAutoAddingFunctionBuilder_B(func_name, code, c)
#define FaseAutoAddingFunctionBuilder(func_name, code) \
    FaseAutoAddingFunctionBuilder_(func_name, code, __COUNTER__)

#else  // ifdef __COUNTER__

#define FaseAutoAddingFunctionBuilder(func_name, code) \
    FaseAutoAddingFunctionBuilder_B(func_name, code, 0)

#endif

}  // namespace fase

#endif  // CONSTEXPR_STRING_H_20180926
