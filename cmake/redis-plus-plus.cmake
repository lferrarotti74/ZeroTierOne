set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    redis-plus-plus
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG 1.3.15
    GIT_SHALLOW ON
)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE INTERNAL "Build static library" FORCE)
# We only link redis++::redis++_static, so skip the unused shared library.
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE INTERNAL "Build shared library" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE INTERNAL "Build tests" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC_WITH_PIC ON CACHE INTERNAL "Build static library with PIC" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build shared libraries" FORCE)

FetchContent_MakeAvailable(redis-plus-plus)
if(NOT TARGET redis++::redis++_static)
    message(FATAL_ERROR "A required redis-plus-plus target (redis++::redis++_static) was not imported")
endif()

# redis++'s static library calls into hiredis but does not propagate that
# dependency to consumers, so the final link fails with undefined hiredis
# symbols (redisAppendCommand, redisFree, freeReplyObject, ...). Attach hiredis
# to the static target's interface. find_library (not find_package(hiredis))
# because Debian's libhiredis-dev ships no CMake config.
find_library(HIREDIS_LIB hiredis REQUIRED)
target_link_libraries(redis++_static INTERFACE ${HIREDIS_LIB})
message(STATUS "redis-plus-plus imported (hiredis: ${HIREDIS_LIB})")
