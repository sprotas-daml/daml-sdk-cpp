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
  message(STATUS 
    "Installing vcpkg dependencies... Command to execute: ${VCPKG_EXECUTABLE} install")
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

project(dog-indexer LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Format.cmake)

# Find dependencies
find_package(tomlplusplus CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)

add_subdirectory(libs/daml-sdk-extension)

add_executable(exe)
target_compile_features(exe PUBLIC cxx_std_23)

target_compile_definitions(exe PRIVATE
  "$<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>"
  "$<$<NOT:$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>"
)

target_sources(exe
    PRIVATE
        src/main.cpp
        src/app.impl.cpp
        src/processors/requester.impl.cpp
        src/processors/manager.impl.cpp
        src/processors/parser.impl.cpp
        src/processors/db_writer.impl.cpp
        src/processors/converter.impl.cpp
        src/processors/pipeline.router.impl.cpp
        src/modules/hold/database.hold.impl.cpp
        src/modules/hold/converter.hold.impl.cpp
        src/utils.impl.cpp
        src/database.impl.cpp
        src/config.impl.cpp
        
        src/workers/token_scan/token_scan.impl.cpp
        src/workers/token_scan/database.token.impl.cpp
        src/workers/api_core.impl.cpp
    PUBLIC
      FILE_SET cxx_modules TYPE CXX_MODULES FILES
        src/utils.cppm
        src/database.cppm
        src/config.cppm
        src/processors/requester.cppm
        src/processors/manager.cppm
        src/processors/parser.cppm
        src/processors/db_writer.cppm
        src/processors/converter.cppm
        src/app.cppm
        src/processors/pipeline.domain_traits.cppm
        src/processors/pipeline.router.cppm
        src/modules/hold/database.hold.cppm
        src/modules/hold/converter.hold.cppm
        src/utils/concurrent_queue.cppm
        src/workers/token_scan/token_scan.cppm
        src/workers/token_scan/database.token.cppm
        src/workers/api.cppm
        src/workers/api_core.cppm
        src/workers/token_scan/api.cppm
)

target_include_directories(exe PRIVATE src)

target_link_libraries(exe PRIVATE
    tomlplusplus::tomlplusplus
    spdlog::spdlog
    fmt::fmt
    libpqxx::pqxx
    cpr::cpr
    nlohmann_json::nlohmann_json
    Crow::Crow
    daml_sdk_ext::daml_sdk_ext
)

add_clang_format_target(exe)
