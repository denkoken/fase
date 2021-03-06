cmake_minimum_required(VERSION 3.8.2)

# ------------------------------------------------------------------------------
# -------------------------------- Check parent --------------------------------
# ------------------------------------------------------------------------------
get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
    message(STATUS "Build Fase as subdirectory mode")
else()
    message(STATUS "Build Fase as independent mode")
endif()

# ------------------------------------------------------------------------------
# ----------------------------------- Options ----------------------------------
# ------------------------------------------------------------------------------
set(FASE_BUILD_THIRD_PARTY ON CACHE BOOL "Build third pary libraries")
set(FASE_BUILD_GLFW ON CACHE BOOL "Build glfw libraries and link")
set(FASE_BUILD_EXAMPLES ON CACHE BOOL "Build examples")
set(FASE_BUILD_TESTS ON CACHE BOOL "Build tests")
set(FASE_EXTERNAL_INCLUDE "" CACHE STRING
    "External directories for third party")
set(FASE_EXTERNAL_LIBRARY "" CACHE STRING "External libraries for third party")

message(STATUS "Build third_party: ${FASE_BUILD_THIRD_PARTY},"
               "glfw: ${FASE_BUILD_GLFW},"
               "examples: ${FASE_BUILD_EXAMPLES}, tests: ${FASE_BUILD_TESTS}")
message(STATUS "External include directories: ${FASE_EXTERNAL_INCLUDE}")
message(STATUS "External link libraries: ${FASE_EXTERNAL_LIBRARY}")

# ------------------------------------------------------------------------------
# ----------------------------------- Common -----------------------------------
# ------------------------------------------------------------------------------
project(fase CXX C)

set(CMAKE_CXX_STANDARD 17)  # C++ 17

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
    set(warning_options "-Wall -Wextra -Wcast-align -Wcast-qual"
                        "-Wctor-dtor-privacy -Wdisabled-optimization"
                        "-Wformat=2 -Winit-self"
                        "-Wmissing-declarations -Wmissing-include-dirs"
                        "-Woverloaded-virtual -Wredundant-decls -Wshadow"
                        "-Wsign-conversion -Wsign-promo"
                        "-Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(warning_options "--pedantic -Wall -Wextra -Wcast-align -Wcast-qual"
                        "-Wctor-dtor-privacy -Wdisabled-optimization"
                        "-Wformat=2 -Winit-self -Wlogical-op"
                        "-Wmissing-declarations -Wmissing-include-dirs"
                        "-Wnoexcept -Woverloaded-virtual"
                        "-Wredundant-decls -Wshadow -Wsign-conversion"
                        "-Wsign-promo -Wstrict-null-sentinel -Wno-noexcept"
                        "-Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
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
    set(compile_flags "${warning_options}")
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${compile_flags})
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
list(APPEND FASE_LIBRARY ${FASE_EXTERNAL_LIBRARY})

if (FASE_BUILD_THIRD_PARTY)
    # OpenGL
    find_package(OpenGL REQUIRED)
    list(APPEND FASE_INCLUDE ${OPENGL_INCLUDE_DIR})
    list(APPEND FASE_LIBRARY ${OPENGL_LIBRARY})

    # GL3W
    add_library(gl3w ${LINK_TYPE}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w/GL/gl3w.c
    )
    if (APPLE)
        SET_TARGET_PROPERTIES(gl3w PROPERTIES LINK_FLAGS
                              "-framework CoreFoundation")
    endif()
    setup_target_simple(gl3w ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/examples/libs/gl3w "${OPENGL_LIBRARY}")
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

        list(APPEND FASE_LIBRARY glfw)
        list(APPEND IMGUI_LIBRARY glfw)
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

    # Json11
    add_library(json11 ${LINK_TYPE}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json11/json11.cpp
    )
    list(APPEND JSON11_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json11)
    setup_target_simple(json11 "${JSON11_INCLUDE_DIRS}" "")
    list(APPEND FASE_INCLUDE ${JSON11_INCLUDE_DIRS})
    list(APPEND FASE_LIBRARY json11)

    # OpenCV (not necessary)
    find_package(OpenCV)

    # Native File Dialog
    set(nfd_PLATFORM Undefined)
    if(WIN32)
      set(nfd_PLATFORM PLATFORM_WIN32)
    elseif(APPLE)
      set(nfd_PLATFORM PLATFORM_MACOS)
    elseif(UNIX AND NOT APPLE)
      set(nfd_PLATFORM PLATFORM_LINUX)
    endif()

    set(NFD_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/include/nfd.h
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/nfd_common.c
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/common.h
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/simple_exec.h
        )

    if(nfd_PLATFORM STREQUAL PLATFORM_WIN32)
      list(APPEND NFD_SOURCE_FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/nfd_win.cpp)
    endif()

    if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
      list(APPEND NFD_SOURCE_FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/nfd_gtk.c)
    endif()

    if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
      find_library(APPKIT_LIBRARY AppKit)
      list(APPEND NFD_SOURCE_FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/nfd_cocoa.m)
    endif()

    add_library(nfd ${LINK_TYPE} ${NFD_SOURCE_FILES})

    target_include_directories(nfd
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/include)

    if(nfd_PLATFORM STREQUAL PLATFORM_LINUX)
      target_include_directories(nfd PRIVATE ${GTK3_INCLUDE_DIRS})
      target_link_libraries(nfd PRIVATE ${GTK3_LIBRARIES})
    endif()

    if(nfd_PLATFORM STREQUAL PLATFORM_MACOS)
      target_link_libraries(nfd PRIVATE ${APPKIT_LIBRARY})
    endif()
    list(APPEND FASE_LIBRARY nfd)

    # Catch2
    list(APPEND FASE_INCLUDE
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Catch2/single_include)
endif()

# ------------------------------------------------------------------------------
# ----------------------------- Internal Libraries -----------------------------
# ------------------------------------------------------------------------------

# fase.so
add_library(fase ${LINK_TYPE}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/type_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/code_gen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/imgui_editor/imgui_editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/imgui_editor/pipe_edit_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/imgui_editor/setup_var_editors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/imgui_editor/links_view.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fase2/imgui_editor/imgui_commons.cpp
)

setup_target(fase "${FASE_INCLUDE}" "${FASE_LIBRARY}")
list(APPEND FASE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND FASE_LIBRARY fase)

# ---------------------------------- examples ----------------------------------
if (FASE_BUILD_EXAMPLES)
    set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")

    # GUI Editor for first touch.
    add_executable(guieditor_for_first_touch
        ${EXAMPLES_DIR}/guieditor_for_first_touch/main.cpp
    )
    list(APPEND EXAMPLES guieditor_for_first_touch)

    # GUI Editor
    add_executable(guieditor ${EXAMPLES_DIR}/guieditor/main.cpp)
    list(APPEND EXAMPLES guieditor)

    # Standard Parts
    add_executable(fixed         ${EXAMPLES_DIR}/std_parts/fixed.cpp)
    add_executable(exportable    ${EXAMPLES_DIR}/std_parts/exportable.cpp)
    add_executable(hard_callable ${EXAMPLES_DIR}/std_parts/hard_callable.cpp)
    list(APPEND EXAMPLES fixed)
    list(APPEND EXAMPLES exportable)
    list(APPEND EXAMPLES hard_callable)

    # GUI Editor with OpenCV
    if (${OpenCV_FOUND})
        add_executable(guieditor_opencv
            ${EXAMPLES_DIR}/guieditor/main_opencv.cpp
        )
        list(APPEND FASE_INCLUDE ${OpenCV_INCLUDE_DIRS})  # Append OpenCV
        list(APPEND FASE_LIBRARY ${OpenCV_LIBS})
        list(APPEND EXAMPLES guieditor_opencv)
    endif()

    foreach(target IN LISTS EXAMPLES)
        setup_target(${target} "${FASE_INCLUDE}" "${FASE_LIBRARY}")
    endforeach()
endif()

# ----------------------------------- tests ------------------------------------
if (FASE_BUILD_TESTS)
    add_executable(fase_test
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_variable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_function.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_core.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_core_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_fase.cpp
    )
    setup_target(fase_test "${FASE_INCLUDE}" "${FASE_LIBRARY}")
endif()

# ------------------------------------------------------------------------------
# ----------------------------- Pass to the parent -----------------------------
# ------------------------------------------------------------------------------
if(HAS_PARENT)
    set(FASE_INCLUDE ${FASE_INCLUDE} PARENT_SCOPE)
    set(FASE_LIBRARY ${FASE_LIBRARY} PARENT_SCOPE)
endif()
