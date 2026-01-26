cmake_minimum_required(VERSION 3.15)
project(GtkAppFolder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Include directories
include_directories(${GTK3_INCLUDE_DIRS})

# Link directories
link_directories(${GTK3_LIBRARY_DIRS})

# Add definitions
add_definitions(${GTK3_CFLAGS_OTHER})

# Source files
set(SOURCES
    src/main.cpp
    src/AppImageBrowser.cpp
)

set(HEADERS
    src/AppImageBrowser.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${GTK3_LIBRARIES})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
