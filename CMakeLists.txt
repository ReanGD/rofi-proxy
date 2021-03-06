cmake_minimum_required(VERSION 3.10)
project(rofi_proxy VERSION 0.4 LANGUAGES CXX)

find_package(PkgConfig)
pkg_search_module(CAIRO REQUIRED cairo)
pkg_search_module(GLIB2 REQUIRED glib-2.0)
pkg_get_variable(ROFI_PLUGINS_DIR rofi pluginsdir)


file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
  PRIVATE
    ${GLIB2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Werror

    -Wall
    -Wextra
    -Wpedantic

    -Wcast-qual
    -Wcast-align
    -Wsign-promo
    -Wconversion
    -Wfloat-equal
    -Wenum-compare
    -Wold-style-cast
    -Wredundant-decls
    -Wsign-conversion
    -Wctor-dtor-privacy
    -Woverloaded-virtual

    -Wno-format-security
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

install(
  TARGETS ${PROJECT_NAME}
  DESTINATION ${ROFI_PLUGINS_DIR}
)
