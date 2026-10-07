cmake_minimum_required(VERSION 3.15)
project(veche LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к LLVM (ЗАМЕНИ НА СВОЙ ПУТЬ!)
set(LLVM_DIR "G:/clang+llvm-23.1.3-x86_64-pc-windows-msvc/clang+llvm-23.1.3-x86_64-pc-windows-msvc/lib/cmake/llvm")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs
    core support irreader target mc mcparser
    passes transformutils analysis bitwriter
)

# Собираем все .cpp из src/
file(GLOB VECHE_SOURCES "src/*.cpp")

add_executable(veche ${VECHE_SOURCES})
target_link_libraries(veche PRIVATE ${llvm_libs} gdi32)
target_include_directories(veche PRIVATE src)

# Runtime для скомпилированных программ
add_library(veche_runtime STATIC veche_runtime.c)

# Установка путей для копирования
set_target_properties(veche PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)