cmake_minimum_required(VERSION 3.15)
project(tinytap C)

#
# Plattform-Erkennung
#
if(WIN32)
    message(STATUS "Building for Windows")
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
    set(NET_LIBS ws2_32)
endif()
if(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    message(STATUS "Building for macOS")
    set(PLATFORM_MAC TRUE)
    add_definitions(-DPLATFORM_MAC)
endif()

#
# Quellcode-Dateien
#
set(SOURCES
    src/main.c
    src/para.c
    src/tnp.c
)

#
# Include-Pfade
#
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Erzeuge die ausführbare Datei
add_executable(tinytap ${SOURCES})

#
# Plattform-spezifische Libraries
#
if(PLATFORM_WINDOWS)
    target_link_libraries(tinytap ${NET_LIBS})
endif()
if(PLATFORM_LINUX OR PLATFORM_MAC)
    find_package(Threads REQUIRED)
    target_link_libraries(tinytap PRIVATE Threads::Threads)
endif()

#
# Compiler-Optionen
#
target_compile_options(tinytap PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

#
# Optional: statischer Build unter Windows
#
if(PLATFORM_WINDOWS)
    set_property(TARGET tinytap PROPERTY LINK_FLAGS "-static")
endif()

#
# Ausgabe des Zielsystems
#
message(STATUS "Target: ${CMAKE_SYSTEM_NAME}")


#*** EOF ***