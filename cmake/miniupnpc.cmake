set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    miniupnpc
    URL https://github.com/miniupnp/miniupnp/releases/download/miniupnpc_2_3_3/miniupnpc-2.3.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(UPNPC_BUILD_TESTS FALSE CACHE INTERNAL "Build tests" FORCE)
set(UPNPC_BUILD_SHARED FALSE CACHE INTERNAL "Build shared library" FORCE)
set(UPNPC_BUILD_STATIC TRUE CACHE INTERNAL "Build static library" FORCE)
set(UPNPC_BUILD_SAMPLE FALSE CACHE INTERNAL "Build sample" FORCE)

FetchContent_MakeAvailable(miniupnpc)

if (NOT TARGET miniupnpc::miniupnpc)
     message(FATAL_ERROR "A required miniupnpc target (miniupnpc::miniupnpc) was not imported")
endif()
add_definitions(-DZT_USE_MINIUPNPC)

FetchContent_Declare(
    libnatpmp
    GIT_REPOSITORY https://github.com/miniupnp/libnatpmp.git
    GIT_TAG master
    GIT_SHALLOW ON
    PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/ext/cmake-patches/libnatpmp.patch
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(libnatpmp)

if (NOT TARGET natpmp)
     message(FATAL_ERROR "A required libnatpmp target (natpmp) was not imported")
endif()
