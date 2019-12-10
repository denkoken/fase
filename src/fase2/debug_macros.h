
#ifndef DEBUG_MACROS_H_20191210
#define DEBUG_MACROS_H_20191210

#include <iostream>

#define FASE_DEBUG_LOC(loc)
#define FASE_COMMA_DEBUG_LOC(loc)
#define FASE_DEBUG_LOC_LOG(loc, str) (void)(str)

#ifndef NDEBUG

#if __has_include(<source_location>)
#include <source_location>
using std_source_location = std::source_location;
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
using std_source_location = std::experimental::source_location;
#else
#define NO_SOURCE_LOCATION
#endif

#if !defined(NO_SOURCE_LOCATION) &&                                            \
        (defined(__cpp_lib_source_location) ||                                 \
         defined(__cpp_lib_experimental_source_location))

#undef FASE_DEBUG_LOC
#undef FASE_COMMA_DEBUG_LOC
#undef FASE_DEBUG_LOC_LOG

#define FASE_DEBUG_LOC(loc)                                                    \
    std_source_location loc = std_source_location::current()
#define FASE_COMMA_DEBUG_LOC(loc)                                              \
    , std_source_location loc = std_source_location::current()
#define FASE_DEBUG_LOC_LOG(loc, str)                                           \
    std::cout << loc.file_name() << ":" << loc.line() << "  " << str           \
              << std::endl

#define FASE_IS_DEBUG_SOURCE_LOCATION_ON

#endif

#endif // #ifndef NDEBUG

#endif // DEBUG_MACROS_H_20191210
