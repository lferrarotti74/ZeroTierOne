cmake_minimum_required(VERSION 3.13)
project(service)

file(GLOB service_src_glob ${PROJ_DIR}/service/*.cpp)
file(GLOB service_hdr_glob ${PROJ_DIR}/service/*.hpp)

add_library(zerotier-service STATIC ${service_src_glob} ${service_hdr_glob})

set(INCLUDE_DIRS
    "${httplib_SOURCE_DIR}"
    ${RUSTYBITS_INCLUDE_DIR}
)

# No OpenSSL here: the control plane is plain-HTTP cpp-httplib and SSO TLS is in rustybits.
# The executable links OpenSSL only where actually needed (controller / Linux rustybits)
# -- see the top-level CMakeLists.
set(LINK_LIBS
    zerotier-osdep
    zerotier-core
    prometheus-cpp-lite
    nlohmann_json::nlohmann_json
    Threads::Threads
    opentelemetry-cpp::api
    Threads::Threads
    rustybits
)

if(ZT1_CENTRAL_CONTROLLER)
    add_definitions(-DZT_CONTROLLER_USE_LIBPQ)
    list(APPEND INCLUDE_DIRS
        ${PostgreSQL_INCLUDE_DIRS}
        "${redis++_BUILD_DIR}/src"
        ${pqxx_INCLUDE_DIRS}
    )
    list(APPEND LINK_LIBS
        pqxx
        redis++::redis++_static
        # OneService.cpp includes the OTLP exporter headers, which pull in the
        # generated *.pb.h. OTel (>= 1.22) stamps those with OPENTELEMETRY_PROTO_API --
        # a PUBLIC compile definition carried by the opentelemetry_proto target -- so
        # this library must link the exporter targets (which transitively bring
        # opentelemetry_proto), not just ::sdk, or the macro is undefined when compiling
        # OneService.cpp. (1.21 didn't stamp it, which is why bumping OTel surfaced this.)
        opentelemetry-cpp::sdk
        opentelemetry-cpp::trace
        opentelemetry-cpp::proto_grpc
        opentelemetry-cpp::otlp_grpc_client
        opentelemetry-cpp::otlp_grpc_exporter
        opentelemetry-cpp::otlp_grpc_log_record_exporter
        opentelemetry-cpp::otlp_grpc_metrics_exporter
        opentelemetry-cpp::otlp_http_exporter
        opentelemetry-cpp::otlp_http_log_record_exporter
        opentelemetry-cpp::otlp_http_metric_exporter
    )
endif()

target_include_directories(zerotier-service
    PRIVATE
        ${INCLUDE_DIRS}
)


target_link_libraries(zerotier-service 
    ${LINK_LIBS}
)
