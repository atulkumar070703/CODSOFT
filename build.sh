#!/bin/bash

# AngelOne WebSocket Client - Build Script
# This script builds the AngelOne WebSocket client

set -e  # Exit on error

echo "========================================"
echo "AngelOne WebSocket Client - Build Script"
echo "========================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if dependencies are installed
check_dependencies() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    MISSING_DEPS=()
    
    # Check for g++
    if ! command -v g++ &> /dev/null; then
        MISSING_DEPS+=("g++")
    fi
    
    # Check for cmake
    if ! command -v cmake &> /dev/null; then
        MISSING_DEPS+=("cmake")
    fi
    
    # Check for Boost (by trying to compile a test program)
    if ! echo '#include <boost/system/error_code.hpp>' | g++ -x c++ - -c 2>/dev/null; then
        MISSING_DEPS+=("libboost-all-dev")
    fi
    
    # Check for OpenSSL
    if ! echo '#include <openssl/ssl.h>' | g++ -x c++ - -c 2>/dev/null; then
        MISSING_DEPS+=("libssl-dev")
    fi
    
    # Check for nlohmann/json
    if ! echo '#include <nlohmann/json.hpp>' | g++ -x c++ - -c 2>/dev/null; then
        MISSING_DEPS+=("nlohmann-json3-dev")
    fi
    
    if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
        echo -e "${RED}Missing dependencies:${NC}"
        for dep in "${MISSING_DEPS[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo -e "${YELLOW}To install on Ubuntu/Debian, run:${NC}"
        echo "sudo apt-get update && sudo apt-get install -y ${MISSING_DEPS[*]}"
        exit 1
    fi
    
    echo -e "${GREEN}All dependencies found!${NC}"
}

# Build using CMake
build_cmake() {
    echo -e "${YELLOW}Building with CMake...${NC}"
    
    mkdir -p build
    cd build
    cmake ..
    make
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Build successful!${NC}"
        echo -e "${GREEN}Executable created: build/angelone_client${NC}"
        cd ..
    else
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
}

# Alternative: Direct g++ compilation
build_manual() {
    echo -e "${YELLOW}Building with g++ directly...${NC}"
    
    g++ -std=c++17 -pthread \
        angelone_websocket.cpp main.cpp \
        -lboost_system -lssl -lcrypto -lz \
        -o angelone_client
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Build successful!${NC}"
        echo -e "${GREEN}Executable created: ./angelone_client${NC}"
    else
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Options:"
    echo "  cmake       Build using CMake (default)"
    echo "  manual      Build using direct g++ compilation"
    echo "  clean       Remove build files"
    echo "  help        Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0          # Build with CMake"
    echo "  $0 manual   # Build with g++ directly"
    echo "  $0 clean    # Clean build directory"
}

# Clean build files
clean() {
    echo -e "${YELLOW}Cleaning build files...${NC}"
    rm -rf build
    rm -f angelone_client
    rm -f market_data.txt
    rm -f market_data.csv
    echo -e "${GREEN}Clean complete!${NC}"
}

# Main
case "${1:-cmake}" in
    cmake)
        check_dependencies
        build_cmake
        ;;
    manual)
        check_dependencies
        build_manual
        ;;
    clean)
        clean
        ;;
    help|--help|-h)
        show_usage
        ;;
    *)
        echo -e "${RED}Unknown option: $1${NC}"
        show_usage
        exit 1
        ;;
esac

echo ""
echo "========================================"
echo "Next Steps:"
echo "========================================"
echo "1. Get your AngelOne credentials from https://smartapi.angelbroking.com/"
echo "2. Run: ./build/angelone_client <api_key> <access_token> <client_id>"
echo "3. Check market_data.txt and market_data.csv for logged data"
echo "========================================"
