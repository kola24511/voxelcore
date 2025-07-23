project(VoxelEngineSrc)

file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_library(VoxelEngineSrc STATIC ${sources} ${headers})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # specific for vcpkg
    find_package(OpenAL CONFIG REQUIRED)
else()
    find_package(OpenAL REQUIRED)
endif()
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(CURL REQUIRED)
find_package(glfw3 REQUIRED)
if(NOT APPLE)
    find_package(EnTT REQUIRED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(OpenAL CONFIG REQUIRED)
    find_package(vorbis REQUIRED)

    # ❗ Ручная интеграция LuaJIT из локального vcpkg_installed
    set(VCPKG_INSTALLED_PATH "${CMAKE_SOURCE_DIR}/build/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    set(LUAJIT_INCLUDE_DIR "${VCPKG_INSTALLED_PATH}/include/luajit")
    set(LUAJIT_STATIC_LIB "${VCPKG_INSTALLED_PATH}/lib/luajit.lib")
    set(LUAJIT_SHARED_LIB "${VCPKG_INSTALLED_PATH}/lib/lua51.lib")
    set(LUAJIT_DLL       "${VCPKG_INSTALLED_PATH}/bin/lua51.dll")

    if(NOT EXISTS "${LUAJIT_INCLUDE_DIR}/luajit.h")
        message(FATAL_ERROR "LuaJIT header not found at: ${LUAJIT_INCLUDE_DIR}/luajit.h")
    endif()

    if(VCPKG_TARGET_TRIPLET MATCHES "static")
        add_library(luajit STATIC IMPORTED)
        set_target_properties(luajit PROPERTIES
            IMPORTED_LOCATION "${LUAJIT_STATIC_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
        )
    else()
        add_library(luajit SHARED IMPORTED)
        set_target_properties(luajit PROPERTIES
            IMPORTED_LOCATION "${LUAJIT_DLL}"
            IMPORTED_IMPLIB   "${LUAJIT_SHARED_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
        )
    endif()

    add_library(luajit::luajit ALIAS luajit)

else()
    find_package(OpenAL REQUIRED)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(luajit REQUIRED IMPORTED_TARGET luajit)
    pkg_check_modules(vorbis REQUIRED IMPORTED_TARGET vorbis)
    pkg_check_modules(vorbisfile REQUIRED IMPORTED_TARGET vorbisfile)

    add_library(luajit::luajit ALIAS PkgConfig::luajit)
    add_library(vorbis ALIAS PkgConfig::vorbis)
    add_library(vorbisfile ALIAS PkgConfig::vorbisfile)
endif()

find_package(GameNetworkingSockets CONFIG REQUIRED)

# Инклуды
target_include_directories(VoxelEngineSrc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Линковка
target_link_libraries(
    VoxelEngineSrc
    PRIVATE
        glfw
        OpenGL::GL
        GLEW::GLEW
        ZLIB::ZLIB
        PNG::PNG
        CURL::libcurl
        OpenAL::OpenAL
        vorbis
        vorbisfile
        luajit::luajit
        GameNetworkingSockets::shared
    PUBLIC
        glm::glm
)

# Флаги компиляции
target_compile_options(
    VoxelEngineSrc
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:
            /utf-8 /MP /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /W4
            /wd4244 /wd4267 /wd4245 /wd4100 /wd4458 /wd4101 /wd4388 /wd4018
        >
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
            -Wall -Wextra -Wformat-nonliteral -Wcast-align -Wpointer-arith
            -Wundef -Wwrite-strings -Wno-unused-parameter -Wno-sign-compare
            -Wno-unknown-pragmas
        >
)

# Флаги линковки
target_link_options(
    VoxelEngineSrc
    PUBLIC
        $<$<CXX_COMPILER_ID:GNU>:-no-pie>
        $<$<PLATFORM_ID:Windows>:$<$<CXX_COMPILER_ID:Clang>:-static>>
)