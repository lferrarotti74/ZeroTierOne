# inja -- header-only C++ templating engine used by service/OneService.cpp
# (`#include <inja/inja.hpp>`). Homebrew ships no usable CMake config and Debian
# has no package, so we vendor the headers via FetchContent. We populate the
# source only and skip inja's own CMakeLists (its build/test/json machinery) by
# pointing SOURCE_SUBDIR at a non-existent directory. inja consumes
# nlohmann::json, which the build already provides via find_package(nlohmann_json)
# and links into the targets that use inja; we only need inja's headers on the
# include path here (header-only, no link), matching the previous conda setup.
include(FetchContent)
FetchContent_Declare(inja
    GIT_REPOSITORY https://github.com/pantor/inja.git
    GIT_TAG v3.4.0
    GIT_SHALLOW ON
    SOURCE_SUBDIR _skip_inja_cmakelists_)   # populate only; no add_subdirectory
FetchContent_MakeAvailable(inja)

# inja ships an amalgamated header under single_include/inja/inja.hpp.
include_directories("${inja_SOURCE_DIR}/single_include")
