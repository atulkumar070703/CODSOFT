cmake_minimum_required(VERSION 3.10)
project(AngelOneWebSocket)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories for dependencies
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# WebSocketpp is a header-only library, you need to download it
# Download from: https://github.com/zaphoyd/websocketpp
# Or install via package manager
# For now, we assume it's in /usr/include or specified via WEBSOCKETPP_INCLUDE_DIR
if(DEFINED ENV{WEBSOCKETPP_INCLUDE_DIR})
    include_directories($ENV{WEBSOCKETPP_INCLUDE_DIR})
endif()

# nlohmann/json is also header-only
# Install via: sudo apt-get install nlohmann-json3-dev
# Or download from: https://github.com/nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # Try to find system-installed version
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
        PATHS /usr/include /usr/local/include
    )
    if(NLOHMANN_JSON_INCLUDE_DIR)
        include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
    endif()
endif()

# Source files
set(SOURCES
    angelone_websocket.cpp
    main.cpp
)

# Header files
set(HEADERS
    angelone_websocket.h
)

# Create executable
add_executable(angelone_client ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(angelone_client
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
    ssl
    crypto
    z
)

# Compiler flags
target_compile_options(angelone_client PRIVATE
    -Wall
    -Wextra
    -O2
)

# Installation
install(TARGETS angelone_client DESTINATION bin)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "AngelOne WebSocket Client Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Boost Version: ${Boost_VERSION}")
message(STATUS "OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
