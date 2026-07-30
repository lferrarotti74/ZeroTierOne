cmake_minimum_required(VERSION 3.13)
project(ext)

file(GLOB_RECURSE PROM_CORE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/core/include/*.h )
set(PROMETHEUS_CPP_LITE_HEADERS ${PROM_CORE_HEADERS} ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/simpleapi/include)


add_library(prometheus-cpp-lite INTERFACE)
target_sources(prometheus-cpp-lite INTERFACE ${PROMETHEUS_CPP_LITE_HEADERS})
target_include_directories(prometheus-cpp-lite INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/simpleapi/include ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/core/include)
add_custom_target(prometheus-cpp-lite-ide SOURCES ${PROMETHEUS_CPP_LITE_HEADERS})
set(prometheus-cpp-lite_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/simpleapi/include ${CMAKE_CURRENT_SOURCE_DIR}/prometheus-cpp-lite-1.0/core/include PARENT_SCOPE)
