cmake_minimum_required(VERSION 3.17)
project(quickjs C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION BUILD_VERSION)

include(CheckCSourceCompiles)
include(GNUInstallDirs)
find_package(Threads)

option(QUICKJS_ENABLE_LTO "Enable link time optimization" OFF)
option(QUICKJS_ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(QUICKJS_ENABLE_M32 "Force a 32-bit build on x86/x64 compilers that support it" OFF)
option(QUICKJS_ENABLE_ASAN "Enable address sanitizer" OFF)
option(QUICKJS_ENABLE_MSAN "Enable memory sanitizer" OFF)
option(QUICKJS_ENABLE_UBSAN "Enable undefined behavior sanitizer" OFF)
option(QUICKJS_ENABLE_TSAN "Enable thread sanitizer" OFF)
option(QUICKJS_BUILD_SHARED "Build a shared QuickJS library when supported" OFF)
option(QUICKJS_BUILD_FUZZERS "Build libFuzzer targets when supported by the compiler" OFF)
if (MSVC)
    set(QUICKJS_BUILD_TEST262_DEFAULT OFF)
else()
    set(QUICKJS_BUILD_TEST262_DEFAULT ON)
endif()
option(QUICKJS_BUILD_TEST262 "Build the test262 runner" ${QUICKJS_BUILD_TEST262_DEFAULT})

if (MSVC)
    set(QUICKJS_CONFIG_CC cl)
else()
    set(QUICKJS_CONFIG_CC ${CMAKE_C_COMPILER})
endif()

set(SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES
        ${SOURCES_ROOT}/quickjs.h
        ${SOURCES_ROOT}/quickjs-libc.h
        ${SOURCES_ROOT}/quickjs.c
        ${SOURCES_ROOT}/dtoa.c
        ${SOURCES_ROOT}/libregexp.c
        ${SOURCES_ROOT}/libunicode.c
        ${SOURCES_ROOT}/cutils.c
        ${SOURCES_ROOT}/quickjs-libc.c
        )

if(EXISTS ${SOURCES_ROOT}/libbf.c)
    list(APPEND SOURCES ${SOURCES_ROOT}/libbf.c)
endif()

if (WIN32 AND EXISTS ${SOURCES_ROOT}/win_patch.c)
    list(APPEND SOURCES ${SOURCES_ROOT}/win_patch.c)
endif()

add_library(quickjs STATIC ${SOURCES})

if (NOT WIN32)
    set(CMAKE_REQUIRED_QUIET ON)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -I${SOURCES_ROOT}")
    check_c_source_compiles("
        #include \"compat/test-closefrom.c\"
    " HAVE_CLOSEFROM)
endif()

function(quickjs_apply_library_options target)
    target_compile_definitions(${target} PRIVATE
            CONFIG_VERSION="${BUILD_VERSION}"
            _GNU_SOURCE)
    target_include_directories(${target} PUBLIC ${SOURCES_ROOT})

    if (HAVE_CLOSEFROM)
        target_compile_definitions(${target} PRIVATE HAVE_CLOSEFROM)
    endif()
    if (MINGW)
        target_compile_definitions(${target} PRIVATE __USE_MINGW_ANSI_STDIO)
    endif()

    if (Threads_FOUND)
        target_link_libraries(${target} PUBLIC Threads::Threads)
    endif()
    if (NOT WIN32)
        target_link_libraries(${target} PUBLIC m ${CMAKE_DL_LIBS})
    endif()

    if (MSVC)
        target_compile_options(${target} PRIVATE /W3)
        if (QUICKJS_ENABLE_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
                -Wall
                -fwrapv
                $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-array-bounds>
                $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-format-truncation>
                $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-infinite-recursion>
                $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wextra>
                $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-sign-compare>
                $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-missing-field-initializers>
                )
        if (QUICKJS_ENABLE_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if (QUICKJS_ENABLE_LTO)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

    if (NOT MSVC AND QUICKJS_ENABLE_M32)
        target_compile_options(${target} PRIVATE -m32 -msse2 -mfpmath=sse)
        target_link_options(${target} PRIVATE -m32)
    endif()
endfunction()

function(quickjs_apply_executable_options target)
    target_compile_definitions(${target} PRIVATE
            CONFIG_VERSION="${BUILD_VERSION}"
            _GNU_SOURCE)
    target_link_libraries(${target} PRIVATE quickjs)

    if (MINGW)
        target_compile_definitions(${target} PRIVATE __USE_MINGW_ANSI_STDIO)
    endif()

    if (MSVC)
        target_compile_options(${target} PRIVATE /W3)
        if (QUICKJS_ENABLE_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -fwrapv)
        if (QUICKJS_ENABLE_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if (QUICKJS_ENABLE_LTO)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

    if (NOT MSVC AND QUICKJS_ENABLE_M32)
        target_compile_options(${target} PRIVATE -m32 -msse2 -mfpmath=sse)
        target_link_options(${target} PRIVATE -m32)
    endif()
endfunction()

function(quickjs_apply_sanitizer target sanitizer_flag)
    if (MSVC)
        message(WARNING "${sanitizer_flag} is not wired for MSVC in this CMake build")
        return()
    endif()
    target_compile_options(${target} PRIVATE ${sanitizer_flag} -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE ${sanitizer_flag} -fno-omit-frame-pointer)
endfunction()

quickjs_apply_library_options(quickjs)
set(QUICKJS_TARGETS quickjs)

set(QJSC_SOURCES ${SOURCES_ROOT}/qjsc.c)

if (WIN32)
    if (EXISTS ${SOURCES_ROOT}/getopt.c)
        list(APPEND QJSC_SOURCES ${SOURCES_ROOT}/getopt.c)
    endif()
    if (EXISTS ${SOURCES_ROOT}/win_patch.c)
        list(APPEND QJSC_SOURCES ${SOURCES_ROOT}/win_patch.c)
    endif()
endif()

add_executable(qjsc ${QJSC_SOURCES})
quickjs_apply_executable_options(qjsc)
target_compile_definitions(qjsc PRIVATE
        CONFIG_CC="${QUICKJS_CONFIG_CC}"
        CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}"
        )
list(APPEND QUICKJS_TARGETS qjsc)

set(QJS_SOURCES ${SOURCES_ROOT}/qjs.c)

if (NOT MSVC)
    add_custom_command(OUTPUT repl.c
            COMMAND qjsc -s -c -o repl.c -m ${SOURCES_ROOT}/repl.js
            DEPENDS qjsc ${SOURCES_ROOT}/repl.js
            VERBATIM)
    list(APPEND QJS_SOURCES repl.c)
endif()

if (WIN32 AND EXISTS ${SOURCES_ROOT}/win_patch.c)
    list(APPEND QJS_SOURCES ${SOURCES_ROOT}/win_patch.c)
endif()

add_executable(qjs ${QJS_SOURCES})
quickjs_apply_executable_options(qjs)
if (NOT MSVC)
    target_compile_definitions(qjs PRIVATE CONFIG_REPL)
endif()
list(APPEND QUICKJS_TARGETS qjs)

if (QUICKJS_BUILD_TEST262)
    set(TEST262_SOURCES ${SOURCES_ROOT}/run-test262.c)

    if (WIN32 AND EXISTS ${SOURCES_ROOT}/win_patch.c)
        list(APPEND TEST262_SOURCES ${SOURCES_ROOT}/win_patch.c)
    endif()

    add_executable(run-test262 ${TEST262_SOURCES})
    quickjs_apply_executable_options(run-test262)
    list(APPEND QUICKJS_TARGETS run-test262)
endif()

if (QUICKJS_BUILD_SHARED AND NOT WIN32 AND NOT APPLE)
    add_library(quickjs-shared SHARED ${SOURCES})
    set_target_properties(quickjs-shared PROPERTIES OUTPUT_NAME quickjs)
    quickjs_apply_library_options(quickjs-shared)
    list(APPEND QUICKJS_TARGETS quickjs-shared)
endif()

if (QUICKJS_BUILD_FUZZERS AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_library(quickjs-fuzz STATIC ${SOURCES})
    quickjs_apply_library_options(quickjs-fuzz)
    target_compile_options(quickjs-fuzz PRIVATE -fsanitize=fuzzer-no-link)

    foreach(fuzzer IN ITEMS eval compile regexp)
        add_executable(fuzz_${fuzzer}
                ${SOURCES_ROOT}/fuzz/fuzz_${fuzzer}.c
                ${SOURCES_ROOT}/fuzz/fuzz_common.c)
        target_link_libraries(fuzz_${fuzzer} PRIVATE quickjs-fuzz)
        target_compile_options(fuzz_${fuzzer} PRIVATE -fsanitize=fuzzer)
        target_link_options(fuzz_${fuzzer} PRIVATE -fsanitize=fuzzer)
        list(APPEND QUICKJS_TARGETS fuzz_${fuzzer})
    endforeach()
endif()

foreach(target IN LISTS QUICKJS_TARGETS)
    if (QUICKJS_ENABLE_ASAN)
        quickjs_apply_sanitizer(${target} -fsanitize=address)
    endif()
    if (QUICKJS_ENABLE_MSAN)
        quickjs_apply_sanitizer(${target} -fsanitize=memory)
    endif()
    if (QUICKJS_ENABLE_UBSAN)
        quickjs_apply_sanitizer(${target} -fsanitize=undefined)
    endif()
    if (QUICKJS_ENABLE_TSAN)
        quickjs_apply_sanitizer(${target} -fsanitize=thread)
    endif()
endforeach()

install(TARGETS qjs qjsc quickjs
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs)
install(FILES quickjs.h quickjs-libc.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/quickjs)
