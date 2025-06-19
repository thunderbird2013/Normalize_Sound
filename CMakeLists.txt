cmake_minimum_required(VERSION 3.16)
project(NormalizeMP3 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-Flags (nur für Kompilierung)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode -O3 -flto")

# Linker-Flags (nur für Linkschritt!)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libstdc++ -static-libgcc")

# Executable
add_executable(normalize main.cpp)
