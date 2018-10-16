cmake_minimum_required(VERSION 3.8.2)

# ------------------------------------------------------------------------------
# ----------------------------------- Options ----------------------------------
# ------------------------------------------------------------------------------
set(FASE_BUILD_GLFW ON CACHE BOOL "Build glfw libraries and link")
set(FASE_BUILD_EXAMPLES_AND_TESTS ON CACHE BOOL "Build examples and tests")
set(FASE_USE_CPP17 OFF CACHE BOOL "Use c++17 for further feathers")
set(FASE_EXTERNAL_INCLUDE "" CACHE PATH "External directories for third party")
message(STATUS "Build glfw: ${FASE_BUILD_GLFW}, examples and tests: ${FASE_BUILD_EXAMPLES_AND_TESTS}, Use c++17: ${FASE_USE_CPP17}, External include directories: ${FASE_EXTERNAL_INCLUDE}")

# ------------------------------------------------------------------------------
# ----------------------------------- Common -----------------------------------
# ------------------------------------------------------------------------------
project(fase CXX C)

if (FASE_USE_CPP17)
    set(CMAKE_CXX_STANDARD 17)  # C++ 17
else()
    set(CMAKE_CXX_STANDARD 14)  # C++ 14
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(LINK_TYPE STATIC)
else()
    set(LINK_TYPE SHARED)
endif()

# Print make commands for debug
# set(CMAKE_VERBOSE_MAKEFILE 1)

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers)
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
                         -Wsign-promo  -Wno-old-style-cast\
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

# External include directory (assuming external imgui without examples)
list(APPEND FASE_INCLUDE ${FASE_EXTERNAL_INCLUDE})

# For examples
if (FASE_BUILD_EXAMPLES_AND_TESTS)
    # OpenGL
    find_package(OpenGL REQUIRED)
    list(APPEND FASE_INCLUDE ${OPENGL_INCLUDE_DIR})
    list(APPEND FASE_LIBRARY ${OPENGL_LIBRARY})

    # GL3W
    add_library(gl3w ${LINK_TYPE}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w/GL/gl3w.c
    )
    if (APPLE)
        SET_TARGET_PROPERTIES(gl3w PROPERTIES LINK_FLAGS "-framework CoreFoundation")
    endif()
    setup_target_simple(gl3w ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w "")
    list(APPEND FASE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w)
    list(APPEND FASE_LIBRARY gl3w)
    list(APPEND IMGUI_LIBRARY gl3w)

    # GLFW3
    if (FASE_BUILD_GLFW)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
        set(GLFW_INSTALL OFF CACHE BOOL "")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw)
        list(APPEND FASE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include)
        list(APPEND FASE_LIBRARY glfw)
        list(APPEND IMGUI_LIBRARY glfw)
    else()
        find_package(GLFW3 REQUIRED)
        list(APPEND FASE_LIBRARY GLFW)
        list(APPEND IMGUI_LIBRARY GLFW)
    endif()

    # ImGui
    add_library(imgui ${LINK_TYPE}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/imgui_impl_opengl3.cpp
    )
    list(APPEND IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
                                   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples
                                   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w)
    setup_target_simple(imgui "${IMGUI_INCLUDE_DIRS}" "${IMGUI_LIBRARY}")
    list(APPEND FASE_INCLUDE ${IMGUI_INCLUDE_DIRS})
    list(APPEND FASE_LIBRARY imgui)

    # OpenCV (not necessary)
    find_package(OpenCV)

    # Catch2
    list(APPEND FASE_INCLUDE
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/single_include)
endif()

# ------------------------------------------------------------------------------
# ----------------------------- Internal Libraries -----------------------------
# ------------------------------------------------------------------------------

# fase.so
add_library(fase ${LINK_TYPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/type_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dst_code.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_editor/editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_editor/view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_editor/popups.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_editor/menus.cpp
)

setup_target(fase "${FASE_INCLUDE}" "${FASE_LIBRARY}")
list(APPEND FASE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND FASE_LIBRARY fase)

# ----------------------------- examples and tests -----------------------------
if (FASE_BUILD_EXAMPLES_AND_TESTS)
    # CLI Editor
    # add_executable(clieditor
    #     ${CMAKE_CURRENT_SOURCE_DIR}/examples/clieditor/main.cpp
    # )
    # setup_target(clieditor "${FASE_INCLUDE}" "${FASE_LIBRARY}")

    # GUI Editor
    add_executable(guieditor
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/guieditor/main.cpp
    )
    setup_target(guieditor "${FASE_INCLUDE}" "${FASE_LIBRARY}")

    # GUI Editor with OpenCV
    if (${OpenCV_FOUND})
        add_executable(guieditor_opencv
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/guieditor/main_opencv.cpp
        )
        list(APPEND FASE_INCLUDE ${OpenCV_INCLUDE_DIRS})  # Append OpenCV
        list(APPEND FASE_LIBRARY ${OpenCV_LIBS})
        setup_target(guieditor_opencv "${FASE_INCLUDE}" "${FASE_LIBRARY}")
    endif()

    # Test
    add_executable(fase_test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_variable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_function.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_core.cpp
    )
    setup_target(fase_test "${FASE_INCLUDE}" "${FASE_LIBRARY}")
endif()
