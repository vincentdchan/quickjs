cmake_minimum_required(VERSION 3.17)
project(quickjs C)

set(CMAKE_C_STANDARD 99)

file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION BUILD_VERSION)

set(SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES
        ${SOURCES_ROOT}/quickjs.h
        ${SOURCES_ROOT}/quickjs-libc.h
        ${SOURCES_ROOT}/quickjs.c
        ${SOURCES_ROOT}/libregexp.c
        ${SOURCES_ROOT}/libunicode.c
        ${SOURCES_ROOT}/libbf.c
        ${SOURCES_ROOT}/cutils.c
        ${SOURCES_ROOT}/quickjs-libc.c
        )

add_library(quickjs ${SOURCES})
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )

set(QJSC_SOURCES ${SOURCES_ROOT}/qjsc.c)

IF (WIN32)
list(APPEND QJSC_SOURCES ${SOURCES_ROOT}/getopt.c)
list(APPEND QJSC_SOURCES ${SOURCES_ROOT}/win_patch.c)
ENDIF()

add_executable(qjsc ${QJSC_SOURCES})

target_link_libraries(qjsc PRIVATE quickjs)

target_compile_definitions(qjsc
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )

add_custom_command(OUTPUT repl.c
        COMMAND qjsc -c -o repl.c -m ${SOURCES_ROOT}/repl.js)

set(QJS_SOURCES ${SOURCES_ROOT}/qjs.c
        repl.c)

IF (WIN32)
list(APPEND QJS_SOURCES ${SOURCES_ROOT}/win_patch.c)
ENDIF()

add_executable(qjs ${QJS_SOURCES})

target_link_libraries(qjs PRIVATE quickjs)

target_compile_definitions(qjs
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )

set(TEST262_SOURCES ${SOURCES_ROOT}/run-test262.c)

IF (WIN32)
list(APPEND TEST262_SOURCES ${SOURCES_ROOT}/win_patch.c)
ENDIF()

add_executable(run-test262 ${TEST262_SOURCES})

target_link_libraries(run-test262 PRIVATE quickjs)

target_compile_definitions(run-test262
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )
