cmake_minimum_required(VERSION 3.16)
project(WebViewAppPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# QtWebView provides cross-platform WebView abstraction
find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebView Quick Qml)

# Try to find the component-interfaces package first
find_package(component-interfaces QUIET)

# If not found, use the app/interfaces folder
if(NOT component-interfaces_FOUND)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../app/interfaces)
endif()

# Source files - now simplified without platform-specific code
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/webview_resources.qrc
)

add_library(webview_app SHARED ${SOURCES})

target_include_directories(webview_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

target_link_libraries(webview_app 
    PRIVATE 
        Qt6::Core 
        Qt6::Widgets
        Qt6::WebView
        Qt6::Quick
        Qt6::Qml
        component-interfaces
)

# Set output properties
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

# QML and HTML files are now embedded via webview_resources.qrc
