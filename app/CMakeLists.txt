cmake_minimum_required(VERSION 3.16)
project(LogosApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # Enable Qt's Meta-Object Compiler
set(CMAKE_AUTORCC ON)  # Enable automatic compilation of resource files
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# RPATH settings for macOS
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
else()
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN/lib")
endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Add interfaces directory to include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add logos_core include directories - use a variable for the core path
set(LOGOS_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../logos-liblogos)
include_directories(
    ${LOGOS_CORE_DIR}/host
    ${LOGOS_CORE_DIR}
)

# Set logos_core library path using the variable
link_directories(${LOGOS_CORE_DIR}/build/lib)

set(PROJECT_SOURCES
    main.cpp
    window.h
    window.cpp
    interfaces/IComponent.h
    resources.qrc
)

qt_add_executable(LogosApp
    ${PROJECT_SOURCES}
)

# Link with logos_core library using a more flexible approach
target_link_libraries(LogosApp PRIVATE 
    Qt6::Widgets
    logos_core
)

# Copy the core library to the app's lib directory and fix its install name
if(APPLE)
    add_custom_command(TARGET LogosApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${LOGOS_CORE_DIR}/build/lib/liblogos_core.dylib ${CMAKE_CURRENT_BINARY_DIR}/lib/
        COMMAND install_name_tool -id @rpath/liblogos_core.dylib ${CMAKE_CURRENT_BINARY_DIR}/lib/liblogos_core.dylib
        # Add additional rpath entries for flexibility when moving the binary
        COMMAND install_name_tool -add_rpath "@executable_path/lib" ${CMAKE_CURRENT_BINARY_DIR}/LogosApp
        COMMAND install_name_tool -add_rpath "@loader_path/lib" ${CMAKE_CURRENT_BINARY_DIR}/LogosApp
        COMMENT "Copying logos_core library and fixing installation paths"
    )
elseif(UNIX AND NOT APPLE)
    add_custom_command(TARGET LogosApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${LOGOS_CORE_DIR}/build/lib/liblogos_core.so ${CMAKE_CURRENT_BINARY_DIR}/lib/
    )

    # For Linux, CMake's RPATH settings above should handle most needs
    # Check if patchelf is available as an alternative for more complex cases
    find_program(PATCHELF_PROGRAM patchelf)
    if(PATCHELF_PROGRAM)
        add_custom_command(TARGET LogosApp POST_BUILD
            # Properly escape $ORIGIN for shell command
            COMMAND ${PATCHELF_PROGRAM} --set-rpath "\$ORIGIN/lib:\$ORIGIN/../lib:${CMAKE_CURRENT_BINARY_DIR}/lib" ${CMAKE_CURRENT_BINARY_DIR}/LogosApp
            COMMENT "Setting RPATH with patchelf for Linux"
        )
    else()
        message(STATUS "patchelf not found - using CMake's built-in RPATH settings for Linux")
    endif()
endif()

# Copy the logos_host executable from the core build directory
add_custom_command(TARGET LogosApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${LOGOS_CORE_DIR}/build/bin/logos_host ${CMAKE_CURRENT_BINARY_DIR}/logos_host
    COMMENT "Copying logos_host executable to app directory"
)

# Create bin/plugins directory in build folder
add_custom_command(TARGET LogosApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bin/plugins
    COMMAND ${CMAKE_COMMAND} -E echo "Creating plugins directory at ${CMAKE_CURRENT_BINARY_DIR}/bin/plugins"

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/plugins
    COMMAND ${CMAKE_COMMAND} -E echo "Creating app plugins directory at ${CMAKE_CURRENT_BINARY_DIR}/bin/plugins"
)

# Add logos_dapps directory
# add_subdirectory(${CMAKE_SOURCE_DIR}/logos-app-poc/logos_dapps/main_ui)