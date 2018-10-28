
#ifndef DEBUG_MACROS_H_20181028
#define DEBUG_MACROS_H_20181028

#include <exception>

#ifndef NDEBUG
#define START_TRY(str)             \
    {                              \
        const auto try_name = str; \
        try {
#define END_TRY(str)                                                  \
    }                                                                 \
    catch (std::exception & e) {                                      \
        std::cerr << __FILE__ << ":" << __LINE__ << " : " << try_name \
                  << " throws error ! : " << e.what() << std::endl;   \
        std::terminate();                                             \
    }                                                                 \
    }
#define DEBUG_LOG(var)                                               \
    std::clog << __FILE__ << ":" << __LINE__ << #var << " = " << var \
              << std::endl;
#define PRINT_VECTOR(vec)                    \
    std::clog << #vec << std::endl;          \
    for (auto& v : vec) {                    \
        std::clog << "  " << v << std::endl; \
    }
#else
#define START_TRY(str) {
#define END_TRY() }
#define DEBUG_LOG(var)
#endif

#endif  // DEBUG_MACROS_H_20181028
