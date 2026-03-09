cmake_minimum_required(VERSION 3.28.3)

message(STATUS "CMake version: ${CMAKE_VERSION}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Set macOS SDK path
if(APPLE)
  execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  message(STATUS "macOS SDK path: ${CMAKE_OSX_SYSROOT}")
endif()

# C++ modules require Ninja generator
if(NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(WARNING "C++ modules require Ninja generator. Current generator: ${CMAKE_GENERATOR}")
  message(WARNING "Please reconfigure with: cmake -G Ninja")
endif()

# Initialize git submodules if .gitmodules exists (before project())
if(EXISTS "${CMAKE_SOURCE_DIR}/.gitmodules")
  find_package(Git QUIET)
  if(GIT_FOUND)
    message(STATUS "Checking git submodules...")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
        )

    if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to update git submodules")
    endif()
    message(STATUS "Git submodules updated successfully")
  else()
    message(WARNING "Git not found. Submodules may not be initialized.")
  endif()
endif()

# Set vcpkg paths (before project())
set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg")

# Install dependencies if vcpkg.json exists
if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json")
  message(STATUS "Installing vcpkg dependencies... Command to execute: ${VCPKG_EXECUTABLE} install")
  execute_process(
        COMMAND "${VCPKG_EXECUTABLE}" "install"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE VCPKG_INSTALL_RESULT
    )

  if(NOT VCPKG_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install vcpkg dependencies, script ignores the error and continues to avoid case in which dependencies are already installed")
  endif()
  message(STATUS "vcpkg dependencies installed successfully")
endif()

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(daml-sdk LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")


find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(intx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)


add_library(daml-sdk STATIC)
target_compile_features(daml-sdk PUBLIC cxx_std_23)

target_sources(daml-sdk
    PRIVATE
      src/utils/sign.impl.cpp
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            src/model/datatype.cppm
            src/model/request.cppm
            src/model/response.cppm
            
            src/utils/sign.cppm
            
            src/type/decimal.cppm
            
            src/model/model.cppm
            src/utils/utils.cppm
            src/type/type.cppm
            
            src/daml.cppm
)

target_include_directories(daml-sdk PUBLIC src)

target_link_libraries(daml-sdk
  PUBLIC
    nlohmann_json::nlohmann_json
  PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    fmt::fmt
)

add_library(daml-sdk::daml-sdk ALIAS daml-sdk)


#Testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(run_tests)
target_compile_features(run_tests PUBLIC cxx_std_23)

target_sources(run_tests
  PRIVATE
    tests/test_decimal.cpp)

target_link_libraries(run_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        daml-sdk::daml-sdk
)

include(GoogleTest)
gtest_discover_tests(run_tests)
