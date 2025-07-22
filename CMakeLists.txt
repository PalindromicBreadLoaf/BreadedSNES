cmake_minimum_required(VERSION 3.16)
project(breadedSNES VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
    set(CMAKE_INSTALL_RPATH "@executable_path")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# Find SDL2
if(WIN32 AND NOT DEFINED ENV{VCPKG_ROOT})
    # Windows things
    set(SDL2_DIR "C:/SDL2" CACHE PATH "/path/to/sdl2") # Do this later when I have access to a Windows machine
    find_package(SDL2 REQUIRED CONFIG)
elseif(APPLE)
    # For macOS
    execute_process(
            COMMAND brew --prefix sdl2
            OUTPUT_VARIABLE SDL2_BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )

    if(SDL2_BREW_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${SDL2_BREW_PREFIX}")
    endif()

    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

    find_package(SDL2 REQUIRED CONFIG)

    message(STATUS "Found SDL2 at: ${SDL2_DIR}")
elseif(UNIX)
    if(NOT SDL2_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
    endif()
endif()


add_executable(breadedSNES
        src/main.cpp
        src/cpu.cpp
        src/ppu.cpp
        src/apu.cpp
        src/bus.cpp
        src/system.cpp
        src/system.h
        src/apu.h
        src/bus.h
        src/cpu.h
        src/ppu.h
)

if(SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

target_include_directories(breadedSNES PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
)

# Link libraries
if(WIN32)
    target_link_libraries(breadedSNES
            SDL2::SDL2
            SDL2::SDL2main
    )
else()
    # Unix systems
    target_link_libraries(breadedSNES SDL2::SDL2)

    # macOS only
    if(APPLE AND TARGET SDL2::SDL2main)
        target_link_libraries(breadedSNES SDL2::SDL2main)
    endif()

    # pkg-config
    if(SDL2_LIBRARIES AND NOT TARGET SDL2::SDL2)
        target_link_libraries(breadedSNES ${SDL2_LIBRARIES})
        target_compile_options(breadedSNES PRIVATE ${SDL2_CFLAGS_OTHER})
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(breadedSNES PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(breadedSNES PRIVATE
            /W4
            /wd4100  # Disable unused parameter warning
    )
endif()

# Install
install(TARGETS breadedSNES
        RUNTIME DESTINATION bin
)

# Windows SDL2 Stuff
if(WIN32)
    if(TARGET SDL2::SDL2)
        get_target_property(SDL2_DLL_PATH SDL2::SDL2 IMPORTED_LOCATION)
        if(SDL2_DLL_PATH)
            install(FILES ${SDL2_DLL_PATH} DESTINATION bin)
        endif()
    endif()
endif()

# Package Config
set(CPACK_PACKAGE_NAME "BreadedSNES")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Cross-platform SNES Emulator")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)