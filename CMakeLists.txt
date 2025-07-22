cmake_minimum_required(VERSION 3.21)
project(VoxelEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции сборки
option(VOXELENGINE_BUILD_APPDIR "Pack linux build" OFF)
option(VOXELENGINE_BUILD_TESTS "Build tests" OFF)

# Поддержка vcpkg: если локально, то использовать build/vcpkg_installed
set(VCPKG_INSTALLED_PATH "${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
link_directories("${VCPKG_INSTALLED_PATH}/lib")

# MSVC + clang: статика
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(VCPKG_TARGET_TRIPLET MATCHES "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Поддиректории
add_subdirectory(src)
add_executable(VoxelEngine src/main.cpp)

# Линковка основного движка + winmm на Windows
target_link_libraries(VoxelEngine PRIVATE VoxelEngineSrc
                                          $<$<PLATFORM_ID:Windows>:winmm>)

# GNU: убрать PIE
target_link_options(VoxelEngine PRIVATE $<$<CXX_COMPILER_ID:GNU>:-no-pie>)

# Ресурсы
add_custom_command(
    TARGET VoxelEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/res $<TARGET_FILE_DIR:VoxelEngine>/res
)

# Тесты
if(VOXELENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Доп. проект
add_subdirectory(vctest)