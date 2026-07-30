cmake_minimum_required(VERSION 3.13)
project(zerotier-core LANGUAGES CXX ASM)


file (GLOB core_src_glob ${PROJ_DIR}/node/*.cpp)
file (GLOB core_hdr_glob ${PROJ_DIR}/node/*.hpp)

# Crypto assembly sources are selected by the ZT_USE_*_ASM_* gates set in the
# top-level CMakeLists.txt (which derives them from the target architecture).
if(ZT_USE_X64_ASM_SALSA OR ZT_USE_X64_ASM_ED25519 OR ZT_USE_ARM32_NEON_ASM_CRYPTO)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp -z noexecstack")
endif()

if(ZT_USE_X64_ASM_SALSA)
    set(ASM_SALSA_DIR ${CMAKE_SOURCE_DIR}/ext/x64-salsa2012-asm)
    list(APPEND core_src_glob ${ASM_SALSA_DIR}/salsa2012.s)
    list(APPEND core_hdr_glob ${ASM_SALSA_DIR}/salsa2012.h)
endif()

if(ZT_USE_X64_ASM_ED25519)
    set(ASM_ED25519_DIR ${CMAKE_SOURCE_DIR}/ext/ed25519-amd64-asm)
    list(APPEND core_src_glob
        #${ASM_ED25519_DIR}/batch.c
        ${ASM_ED25519_DIR}/choose_t.s
        ${ASM_ED25519_DIR}/consts.s
        ${ASM_ED25519_DIR}/fe25519_add.s
        ${ASM_ED25519_DIR}/fe25519_freeze.s
        ${ASM_ED25519_DIR}/fe25519_getparity.c
        ${ASM_ED25519_DIR}/fe25519_invert.c
        ${ASM_ED25519_DIR}/fe25519_iseq.c
        ${ASM_ED25519_DIR}/fe25519_iszero.c
        ${ASM_ED25519_DIR}/fe25519_mul.s
        ${ASM_ED25519_DIR}/fe25519_neg.c
        ${ASM_ED25519_DIR}/fe25519_pack.c
        ${ASM_ED25519_DIR}/fe25519_pow2523.c
        ${ASM_ED25519_DIR}/fe25519_setint.c
        ${ASM_ED25519_DIR}/fe25519_square.s
        ${ASM_ED25519_DIR}/fe25519_sub.s
        ${ASM_ED25519_DIR}/fe25519_unpack.c
        ${ASM_ED25519_DIR}/ge25519_add_p1p1.s
        ${ASM_ED25519_DIR}/ge25519_add.c
        ${ASM_ED25519_DIR}/ge25519_base.c
        ${ASM_ED25519_DIR}/ge25519_dbl_p1p1.s
        ${ASM_ED25519_DIR}/ge25519_double_scalarmult.c
        ${ASM_ED25519_DIR}/ge25519_double.c
        ${ASM_ED25519_DIR}/ge25519_isneutral.c
        ${ASM_ED25519_DIR}/ge25519_multi_scalarmult.c
        ${ASM_ED25519_DIR}/ge25519_nielsadd_p1p1.s
        ${ASM_ED25519_DIR}/ge25519_nielsadd2.s
        ${ASM_ED25519_DIR}/ge25519_p1p1_to_p2.s
        ${ASM_ED25519_DIR}/ge25519_p1p1_to_p3.s
        ${ASM_ED25519_DIR}/ge25519_pack.c
        ${ASM_ED25519_DIR}/ge25519_pnielsadd_p1p1.s
        ${ASM_ED25519_DIR}/ge25519_scalarmult_base.c
        ${ASM_ED25519_DIR}/ge25519_unpackneg.c
        ${ASM_ED25519_DIR}/heap_rootreplaced_1limb.s
        ${ASM_ED25519_DIR}/heap_rootreplaced_2limbs.s
        ${ASM_ED25519_DIR}/heap_rootreplaced_3limbs.s
        ${ASM_ED25519_DIR}/heap_rootreplaced.s
        ${ASM_ED25519_DIR}/hram.c
        ${ASM_ED25519_DIR}/index_heap.c
        #${ASM_ED25519_DIR}/keypair.c
        #${ASM_ED25519_DIR}/open.c
        ${ASM_ED25519_DIR}/sc25519_add.s
        ${ASM_ED25519_DIR}/sc25519_barrett.s
        ${ASM_ED25519_DIR}/sc25519_from_shortsc.c
        ${ASM_ED25519_DIR}/sc25519_from32bytes.c
        ${ASM_ED25519_DIR}/sc25519_from64bytes.c
        ${ASM_ED25519_DIR}/sc25519_iszero.c
        ${ASM_ED25519_DIR}/sc25519_lt.s
        ${ASM_ED25519_DIR}/sc25519_mul_shortsc.c
        ${ASM_ED25519_DIR}/sc25519_mul.c
        ${ASM_ED25519_DIR}/sc25519_slide.c
        ${ASM_ED25519_DIR}/sc25519_sub_nored.s
        ${ASM_ED25519_DIR}/sc25519_to32bytes.c
        ${ASM_ED25519_DIR}/sc25519_window4.c
        ${ASM_ED25519_DIR}/sign.c
        ${ASM_ED25519_DIR}/ull4_mul.s
    )
    list(APPEND core_hdr_glob
        ${ASM_ED25519_DIR}/fe25519.h
        ${ASM_ED25519_DIR}/ge25519.h
        ${ASM_ED25519_DIR}/hram.h
        ${ASM_ED25519_DIR}/index_heap.h
        ${ASM_ED25519_DIR}/sc25519.h)
    set_property(SOURCE ${ASM_ED25519_DIR}/fe25519_freeze.s PROPERTY COMPILE_FLAGS "-z noexecstack")
endif()

if(ZT_USE_ARM32_NEON_ASM_CRYPTO)
    set(ASM_ARM32_DIR ${CMAKE_SOURCE_DIR}/ext/arm32-neon-salsa2012-asm)
    list(APPEND core_src_glob ${ASM_ARM32_DIR}/salsa2012.s)
    list(APPEND core_hdr_glob ${ASM_ARM32_DIR}/salsa2012.h)
endif()

# Optimize the hot crypto sources at -O2 even in debug builds; make-linux.mk
# does this because C25519 in particular is unusably slow at -O0/-O1. (No effect
# in release, where everything is already -O3.) Skipped on MSVC, where -O2 means
# /O2 -- incompatible with the /RTC1 runtime checks MSVC injects in Debug; those
# files just build unoptimized in an MSVC Debug build.
if(NOT MSVC)
    set_property(
        SOURCE
            ${PROJ_DIR}/node/Salsa20.cpp
            ${PROJ_DIR}/node/SHA512.cpp
            ${PROJ_DIR}/node/C25519.cpp
            ${PROJ_DIR}/node/Poly1305.cpp
        APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:Debug>:-O2>)
endif()

add_library(zerotier-core STATIC ${core_src_glob} ${core_hdr_glob})

target_include_directories(zerotier-core
    PRIVATE
        ${prometheus-cpp-lite_INCLUDE}
)
target_link_libraries(zerotier-core
    PRIVATE
    nlohmann_json::nlohmann_json
    Threads::Threads
    prometheus-cpp-lite
    Threads::Threads)

# macOS universal: the x64 Salsa20/12 asm must be a single-arch (x86_64) object so it
# lands only in the x86_64 slice of the fat binary (mirrors make-mac.mk's `as -arch
# x86_64`). A separate target with OSX_ARCHITECTURES x86_64 does that under a universal
# configure; Packet.cpp's `ZT_USE_X64_ASM_SALSA2012 && ZT_ARCH_X64` guard keeps the arm64
# slice from referencing it.
if(ZT_MACOS_UNIVERSAL_X64_SALSA_ASM)
    add_library(zt-salsa-x64-asm STATIC ${CMAKE_SOURCE_DIR}/ext/x64-salsa2012-asm/salsa2012.s)
    set_target_properties(zt-salsa-x64-asm PROPERTIES OSX_ARCHITECTURES "x86_64")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/ext/x64-salsa2012-asm/salsa2012.s
        PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
    target_link_libraries(zerotier-core PUBLIC zt-salsa-x64-asm)
endif()
