cmake_minimum_required(VERSION 3.15)
project(GtkAppFolder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find GTK4, libadwaita, and GIO
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(ADW REQUIRED libadwaita-1)
pkg_check_modules(GIO REQUIRED gio-2.0)

# Include directories
include_directories(${GTK4_INCLUDE_DIRS} ${ADW_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})

# Link directories
link_directories(${GTK4_LIBRARY_DIRS} ${ADW_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS})

# Add definitions
add_definitions(${GTK4_CFLAGS_OTHER} ${ADW_CFLAGS_OTHER} ${GIO_CFLAGS_OTHER})

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
target_link_libraries(${PROJECT_NAME} ${GTK4_LIBRARIES} ${ADW_LIBRARIES} ${GIO_LIBRARIES})

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# GSettings schema compile step (for local runtime)
set(SCHEMAS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/schemas)
set(SCHEMAS_BUILD_DIR ${CMAKE_BINARY_DIR}/schemas)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SCHEMAS_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SCHEMAS_SOURCE_DIR}/com.github.gtkappfolder.gschema.xml
            ${SCHEMAS_BUILD_DIR}/
    COMMAND glib-compile-schemas ${SCHEMAS_BUILD_DIR}
    COMMENT "Compiling GSettings schemas to ${SCHEMAS_BUILD_DIR}"
)
