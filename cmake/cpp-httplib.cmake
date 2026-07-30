
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    # v0.47.0+: supports 32-bit Windows. v0.25.0 had a `#warning` refusing 32-bit
    # Windows that MSVC's default preprocessor escalates to a fatal C1021 (breaks
    # the windows-x86 build).
    GIT_TAG v0.47.0
    GIT_SHALLOW ON
)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(HTTPLIB_COMPILE OFF CACHE INTERNAL "")
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE ON CACHE INTERNAL "Use zlib if available")
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE ON CACHE INTERNAL "Use brotli if available")
# The ZeroTier control plane is plain HTTP on localhost (no httplib SSLServer/SSLClient
# anywhere), and outbound SSO/OIDC TLS lives in rustybits (native-tls), so httplib never
# needs OpenSSL. Keeping this ON would link OpenSSL purely as dead weight -- which also
# blocks universal macOS builds (Homebrew OpenSSL is single-arch).
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE INTERNAL "ZeroTier uses httplib for plain HTTP only")
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE ON CACHE INTERNAL "Use zstd if available")
FetchContent_MakeAvailable(cpp-httplib)

if (NOT TARGET httplib::httplib)
     message(FATAL_ERROR "A required cpp-httplib target (cpp-httplib) was not imported")
endif()

