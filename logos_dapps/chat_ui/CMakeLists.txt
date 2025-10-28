cmake_minimum_required(VERSION 3.16)
project(ChatUIPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets RemoteObjects)

# Get the real path to handle symlinks correctly
get_filename_component(REAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

# Try to find the component-interfaces package first
find_package(component-interfaces QUIET)

# If not found, use the app/interfaces folder
if(NOT component-interfaces_FOUND)
    # Include the app/interfaces directory
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
    
    # Create a component-interfaces library
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
endif()

# Check for logos_core library
find_library(LOGOS_CORE_LIB logos_core PATHS ${CMAKE_BINARY_DIR}/../logos-liblogos/lib)

# Source files
set(SOURCES
    ChatUIComponent.cpp
    src/ChatWidget.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api.h
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_client.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_client.h
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_consumer.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_consumer.h
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_provider.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/logos_api_provider.h
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/token_manager.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/token_manager.h
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/module_proxy.cpp
    ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/module_proxy.h
)

# Use umbrella source that includes all generated wrappers to avoid duplicate symbols
set(GENERATED_LOGOS_SDK_CPP ${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/generated/logos_sdk.cpp)
set_source_files_properties(${GENERATED_LOGOS_SDK_CPP} PROPERTIES GENERATED TRUE)
list(APPEND SOURCES ${GENERATED_LOGOS_SDK_CPP})

# Run Logos C++ generator on metadata before compilation
set(REPO_ROOT "${REAL_SOURCE_DIR}/../../..")
set(CPP_GENERATOR "${REPO_ROOT}/build/cpp-generator/bin/logos-cpp-generator")
set(PLUGINS_OUTPUT_DIR "${REPO_ROOT}/modules/build/modules")
set(METADATA_JSON "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json")
if(APPLE)
    set(PLUGIN_SUFFIX ".dylib")
elseif(WIN32)
    set(PLUGIN_SUFFIX ".dll")
else()
    set(PLUGIN_SUFFIX ".so")
endif()

# No explicit dependency plugin path list required; generator handles dependencies via --module-dir

if(EXISTS "${CPP_GENERATOR}" AND EXISTS "${METADATA_JSON}")
    add_custom_command(
        OUTPUT ${GENERATED_LOGOS_SDK_CPP}
        COMMAND "${CPP_GENERATOR}" --metadata "${METADATA_JSON}" --module-dir "${PLUGINS_OUTPUT_DIR}"
        WORKING_DIRECTORY "${REPO_ROOT}"
        DEPENDS ${METADATA_JSON}
        COMMENT "Generating umbrella ${GENERATED_LOGOS_SDK_CPP} and wrappers for chat_ui"
        VERBATIM
    )
    add_custom_target(run_cpp_generator_chat_ui DEPENDS ${GENERATED_LOGOS_SDK_CPP})
else()
    add_custom_target(run_cpp_generator_chat_ui
        COMMAND ${CMAKE_COMMAND} -E echo "logos-cpp-generator or metadata.json not found; skipping generation for chat_ui."
        VERBATIM
    )
endif()

add_library(chat_ui SHARED ${SOURCES})

# Ensure generator runs before building the plugin
add_dependencies(chat_ui run_cpp_generator_chat_ui)

## Include directories
target_include_directories(chat_ui PRIVATE 
   ${CMAKE_CURRENT_SOURCE_DIR}/../../../logos-cpp-sdk/cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../../logos-cpp-sdk/cpp/generated
)

# Link libraries
target_link_libraries(chat_ui
    PRIVATE 
        Qt6::Core 
        Qt6::Widgets
        Qt6::RemoteObjects
        component-interfaces
)

# For macOS, we need to set the bundle properties
if(APPLE)
    set_target_properties(chat_ui PROPERTIES
        BUNDLE FALSE
        FRAMEWORK FALSE
        PREFIX ""
        SUFFIX ".dylib"
    )
else()
    set_target_properties(chat_ui PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
endif()

# Print status messages
message(STATUS "Chat UI Plugin configured successfully") 