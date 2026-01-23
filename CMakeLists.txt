cmake_minimum_required(VERSION 3.5)

project(slither_server CXX)

message(STATUS "** Project: ${PROJECT_NAME}")

# Set CMake library search policy
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0011 NEW)
    if(POLICY CMP0074)
        cmake_policy(SET CMP0074 NEW) # Boost_ROOT
    endif()
endif()

# Compiler flags
if(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601) # Target Windows 7+
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX- /EHsc")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -pedantic")
    if(NOT MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-unused-parameter -Wno-unused-function")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math -funroll-loops -O3")

    # march=native can be problematic in CI environments for generic builds
    # but we keep it for performance if not cross-compiling
    if(NOT CMAKE_CROSSCOMPILING AND NOT MINGW)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
    endif()
endif()

if(MINGW)
    # MinGW might need some extra definitions for Win32 API
    add_definitions(-D_WIN32_WINNT=0x0601)
    # Avoid issues with some boost versions and mingw
    add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS)
endif()

# Source & includes
file(GLOB_RECURSE SOURCE_FILES src/*.cc)
file(GLOB_RECURSE HEADER_FILES src/*.h)

include_directories(src)
include_directories(third_party/websocketpp)

# Dependencies
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.39.0 REQUIRED COMPONENTS system thread program_options)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
    message(STATUS "-- Boost Include: ${Boost_INCLUDE_DIRS}")
    message(STATUS "-- Boost Libraries: ${Boost_LIBRARIES}")
else()
    message(FATAL_ERROR "Failed to find required dependency: Boost")
endif()

# Build
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

if(WIN32)
    # For Windows, we might need ws2_32 and mswsock for asio
    target_link_libraries(${PROJECT_NAME} ws2_32 mswsock)
endif()

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
