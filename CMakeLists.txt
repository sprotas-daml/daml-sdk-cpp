cmake_minimum_required(VERSION 3.28.3)
project(daml-sdk LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND EXISTS "${VCPKG_ROOT}/vcpkg")
    message(STATUS "Installing daml-sdk vcpkg dependencies...")
    execute_process(
        COMMAND "${VCPKG_ROOT}/vcpkg" "install"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE VCPKG_INSTALL_RESULT
    )
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)


add_library(daml-sdk STATIC)
target_compile_features(daml-sdk PUBLIC cxx_std_23)

target_sources(daml-sdk
    PRIVATE
      src/utils/sign.impl.cpp
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            src/model/datatype.cppm
            src/model/choice.cppm
            src/model/request.cppm
            src/model/response.cppm
            
            src/utils/transaction.sign.cppm
            
            src/type/decimal.cppm
            
            src/model/model.cppm
            src/utils/utils.cppm
            src/type/type.cppm
            
            src/daml.cppm
)

target_include_directories(daml-sdk PUBLIC src)

target_link_libraries(daml-sdk PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    fmt::fmt
)

add_library(daml-sdk::daml-sdk)