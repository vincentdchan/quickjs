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

add_executable(qjsc
        ${SOURCES_ROOT}/qjsc.c
        )

target_link_libraries(qjsc PRIVATE quickjs)

target_compile_definitions(qjsc
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )

add_executable(qjs
        ${SOURCES_ROOT}/qjsc.c
        )

target_link_libraries(qjs PRIVATE quickjs)

target_compile_definitions(qjs
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )

add_executable(run-test262
        ${SOURCES_ROOT}/run-test262.c
        )

target_link_libraries(run-test262 PRIVATE quickjs)

target_compile_definitions(run-test262
        PRIVATE
        CONFIG_VERSION="${BUILD_VERSION}"
        )
