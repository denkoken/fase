cmake_minimum_required(VERSION 3.5.1)

# ------------------------------------------------------------------------------
# ----------------------------------- Options ----------------------------------
# ------------------------------------------------------------------------------
option(BUILD_GLFW "Build glfw libraries and link" ON)
option(BUILD_EXAMPLE "Build example codes" ON)
option(BUILD_TEST "Build test codes" ON)
message(STATUS "Build glfw: ${BUILD_GLFW}, examples: ${BUILD_EXAMPLE}, tests: ${BUILD_TEST}")


# ------------------------------------------------------------------------------
# ----------------------------------- Common -----------------------------------
# ------------------------------------------------------------------------------
project(fase CXX C)
set (CMAKE_CXX_STANDARD 14)  # C++ 14

# Print make commands for debug
# set(CMAKE_VERBOSE_MAKEFILE 1)

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/sanitizers)
find_package(Sanitizers) # Address sanitizer (-DSANITIZE_ADDRESS=ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warning options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warning_options "-Wall -Wextra -Wconversion")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(warning_options "-Wall -Wextra -Wcast-align -Wcast-qual \
                         -Wctor-dtor-privacy -Wdisabled-optimization \
                         -Wformat=2 -Winit-self \
                         -Wmissing-declarations -Wmissing-include-dirs \
                         -Wold-style-cast -Woverloaded-virtual \
                         -Wredundant-decls -Wshadow -Wsign-conversion \
                         -Wsign-promo \
                         -Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(warning_options "--pedantic -Wall -Wextra -Wcast-align -Wcast-qual \
                         -Wctor-dtor-privacy -Wdisabled-optimization \
                         -Wformat=2 -Winit-self -Wlogical-op \
                         -Wmissing-declarations -Wmissing-include-dirs \
                         -Wnoexcept -Wold-style-cast -Woverloaded-virtual \
                         -Wredundant-decls -Wshadow -Wsign-conversion \
                         -Wsign-promo -Wstrict-null-sentinel \
                         -Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warning_options "/W4")
else()
    message(WARNING "Unsupported compiler for warning options")
    message ("CMAKE_CXX_COMPILER_ID is ${CMAKE_CXX_COMPILER_ID}")
endif()

# Utility function to setup a target (include, link, warning, sanitizer)
function(setup_target target includes libs)
    target_include_directories(${target} PUBLIC ${includes})
    target_link_libraries(${target} ${libs})
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${warning_options})
    add_sanitizers(${target})
endfunction(setup_target)

# Utility function to setup a target simply (include, link)
function(setup_target_simple target includes libs)
    target_include_directories(${target} PUBLIC ${includes})
    target_link_libraries(${target} ${libs})
endfunction(setup_target_simple)

# ------------------------------------------------------------------------------
# --------------------------- Third Party Libraries ----------------------------
# ------------------------------------------------------------------------------

# For examples
if (BUILD_EXAMPLE)
    # OpenGL
    find_package(OpenGL REQUIRED)
    list(APPEND FASE_INCLUDE ${OPENGL_INCLUDE_DIR})
    list(APPEND FASE_LIBRARY ${OPENGL_LIBRARY})

    # GLFW3
    if (BUILD_GLFW)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
        set(GLFW_INSTALL OFF CACHE BOOL "")
        add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)
        list(APPEND FASE_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/glfw/include)
        list(APPEND FASE_LIBRARY glfw)
    endif()

    # OpenCV
    find_package(OpenCV)
endif()

if (NOT BUILD_GLFW)
    find_package(GLFW3 REQUIRED)
endif()

# ImGui
add_library(imgui SHARED
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/examples/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/examples/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w/GL/gl3w.c
)
set(IMGUI_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/imgui
                       ${CMAKE_SOURCE_DIR}/third_party/imgui/examples
                       ${CMAKE_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w
                       /usr/local/include)

if (BUILD_GLFW)
    setup_target_simple(imgui "${IMGUI_INCLUDE_DIRS}" "glfw")
else()
    setup_target_simple(imgui "${IMGUI_INCLUDE_DIRS}" "GLFW")
endif()
if ((NOT BUILD_GLFW) AND APPLE)
    SET_TARGET_PROPERTIES(imgui PROPERTIES LINK_FLAGS "-framework CoreFoundation")
endif()

list(APPEND FASE_INCLUDE ${IMGUI_INCLUDE_DIRS})
list(APPEND FASE_LIBRARY imgui)

# For tests
if (BUILD_TEST)
    # Catch2
    list(APPEND FASE_INCLUDE
                ${CMAKE_SOURCE_DIR}/third_party/Catch2/single_include)
endif()

# ------------------------------------------------------------------------------
# ----------------------------- Internal Libraries -----------------------------
# ------------------------------------------------------------------------------

# fase.so
add_library(fase SHARED
    ${CMAKE_SOURCE_DIR}/src/fase.cpp
    ${CMAKE_SOURCE_DIR}/src/core.cpp
    ${CMAKE_SOURCE_DIR}/src/core_util.cpp
    ${CMAKE_SOURCE_DIR}/src/type_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/dst_code.cpp
    ${CMAKE_SOURCE_DIR}/src/editor_cli.cpp
    ${CMAKE_SOURCE_DIR}/src/editor_gui.cpp
)
setup_target(fase "${FASE_INCLUDE}" "${FASE_LIBRARY}")
list(APPEND FASE_INCLUDE ${CMAKE_SOURCE_DIR}/src)
list(APPEND FASE_LIBRARY fase)

# ----------------------------------- tests ------------------------------------
if (BUILD_TEST)
    add_executable(test
        ${CMAKE_SOURCE_DIR}/test/test.cpp
        ${CMAKE_SOURCE_DIR}/test/test_variable.cpp
        ${CMAKE_SOURCE_DIR}/test/test_function.cpp
        ${CMAKE_SOURCE_DIR}/test/test_core.cpp
    )
    setup_target(test "${FASE_INCLUDE}" "${FASE_LIBRARY}")
endif()

# ---------------------------------- examples ----------------------------------
if (BUILD_EXAMPLE)
    # CLI Editor
    add_executable(clieditor
        ${CMAKE_SOURCE_DIR}/examples/clieditor/main.cpp
    )
    setup_target(clieditor "${FASE_INCLUDE}" "${FASE_LIBRARY}")

    # GUI Editor
    add_executable(guieditor
        ${CMAKE_SOURCE_DIR}/examples/guieditor/main.cpp
    )
    setup_target(guieditor "${FASE_INCLUDE}" "${FASE_LIBRARY}")

    # GUI Editor with OpenCV
    if (${OpenCV_FOUND})
        add_executable(guieditor_opencv
            ${CMAKE_SOURCE_DIR}/examples/guieditor/main_opencv.cpp
        )
        list(APPEND FASE_INCLUDE ${OpenCV_INCLUDE_DIRS})  # Append OpenCV
        list(APPEND FASE_LIBRARY ${OpenCV_LIBS})
        setup_target(guieditor_opencv "${FASE_INCLUDE}" "${FASE_LIBRARY}")
    endif()
endif()
