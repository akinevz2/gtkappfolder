cmake_minimum_required(VERSION 3.15)
project(GtkAppFolder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option to choose between GTK3 and GTK4
option(USE_GTK4 "Build with GTK4 and libadwaita" ON)

# Find required packages based on GTK version
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

if(USE_GTK4)
    message(STATUS "Building with GTK4 and libadwaita")
    pkg_check_modules(GTK REQUIRED gtk4)
    pkg_check_modules(ADW REQUIRED libadwaita-1)
    pkg_check_modules(GIO REQUIRED gio-2.0)
    set(GTK_COMPILE_DEFS -DUSE_GTK4)
else()
    message(STATUS "Building with GTK3")
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
    pkg_check_modules(GIO REQUIRED gio-2.0)
    set(GTK_COMPILE_DEFS -DUSE_GTK3)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTK_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
)

if(USE_GTK4)
    include_directories(${ADW_INCLUDE_DIRS})
endif()

# Link directories
link_directories(${GTK_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS})
if(USE_GTK4)
    link_directories(${ADW_LIBRARY_DIRS})
endif()

# Add definitions
add_definitions(${GTK_CFLAGS_OTHER} ${GIO_CFLAGS_OTHER} ${GTK_COMPILE_DEFS})
if(USE_GTK4)
    add_definitions(${ADW_CFLAGS_OTHER})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/AppImageBrowser.cpp
    src/core/AppImage.cpp
    src/core/AppImageMetadata.cpp
    src/system/ProcessManager.cpp
    src/system/FileMonitor.cpp
    src/system/Preferences.cpp
    src/ui/UIFactory.cpp
)

set(HEADERS
    src/AppImageBrowser.h
    src/core/AppImage.h
    src/core/AppImageMetadata.h
    src/system/ProcessManager.h
    src/system/FileMonitor.h
    src/system/Preferences.h
    src/ui/UIInterface.h
)

# Add platform-specific UI sources
if(USE_GTK4)
    list(APPEND SOURCES src/ui/GTK4UI.cpp)
    list(APPEND HEADERS src/ui/GTK4UI.h)
else()
    list(APPEND SOURCES src/ui/GTK3UI.cpp)
    list(APPEND HEADERS src/ui/GTK3UI.h)
endif()

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
if(USE_GTK4)
    target_link_libraries(${PROJECT_NAME} 
        ${GTK_LIBRARIES}
        ${ADW_LIBRARIES}
        ${GIO_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
    )
else()
    target_link_libraries(${PROJECT_NAME} 
        ${GTK_LIBRARIES}
        ${GIO_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

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
