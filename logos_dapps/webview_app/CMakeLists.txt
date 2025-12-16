cmake_minimum_required(VERSION 3.16)
project(WebViewAppPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebView Quick Qml RemoteObjects)

if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    get_filename_component(REAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
    set(LOGOS_CPP_SDK_ROOT "${REAL_SOURCE_DIR}/../../../logos-cpp-sdk/result")
endif()

if(NOT EXISTS "${LOGOS_CPP_SDK_ROOT}/include/cpp/logos_api.h")
    message(FATAL_ERROR "logos-cpp-sdk headers not found at ${LOGOS_CPP_SDK_ROOT}. Set LOGOS_CPP_SDK_ROOT to the installed SDK prefix or run git submodule update --init --recursive followed by the SDK build.")
endif()

message(STATUS "Using prebuilt logos-cpp-sdk at: ${LOGOS_CPP_SDK_ROOT}")

find_package(component-interfaces QUIET)

if(NOT component-interfaces_FOUND)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
endif()

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/webview_resources.qrc
)

add_library(webview_app SHARED ${SOURCES})

target_include_directories(webview_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${LOGOS_CPP_SDK_ROOT}/include
    ${LOGOS_CPP_SDK_ROOT}/include/cpp
    ${LOGOS_CPP_SDK_ROOT}/include/core
)

find_library(LOGOS_SDK_LIB logos_sdk PATHS ${LOGOS_CPP_SDK_ROOT}/lib NO_DEFAULT_PATH REQUIRED)

target_link_libraries(webview_app 
    PRIVATE 
        Qt6::Core 
        Qt6::Widgets
        Qt6::WebView
        Qt6::Quick
        Qt6::Qml
        Qt6::RemoteObjects
        component-interfaces
        ${LOGOS_SDK_LIB}
)

if(APPLE)
    set_target_properties(webview_app PROPERTIES
        BUNDLE FALSE
        FRAMEWORK FALSE
        PREFIX ""
        SUFFIX ".dylib"
    )
else()
    set_target_properties(webview_app PROPERTIES
        PREFIX ""
        SUFFIX ".so"
    )
endif()
