cmake_minimum_required(VERSION 3.28.3)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake" 
    CACHE STRING "Vcpkg toolchain file")

set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

include(FetchContent)

FetchContent_Declare(
    daml-sdk
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../daml-sdk
    OVERRIDE_FIND_PACKAGE
)

# FetchContent_Declare(
#     daml-sdk
#     GIT_REPOSITORY https://github.com/daml-sdk/daml-sdk.git
#     GIT_TAG        v1.0.0 
# )
# 
project(daml-sdk-extension LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(daml-sdk CONFIG REQUIRED)

add_library(daml_sdk_ext STATIC)
target_compile_features(daml_sdk_ext PUBLIC cxx_std_23)

target_sources(daml_sdk_ext
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            src/daml_ext.cppm
)

target_include_directories(daml_sdk_ext PUBLIC src)

target_link_libraries(daml_sdk_ext 
    PUBLIC 
        daml-sdk::daml-sdk 
)

add_library(daml_sdk_ext::daml_sdk_ext ALIAS daml_sdk_ext)