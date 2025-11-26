cmake_minimum_required(VERSION 3.16)

# Enable Objective-C++ for macOS
if(APPLE)
    project(WebViewAppPlugin VERSION 1.0.0 LANGUAGES CXX OBJCXX)
else()
    project(WebViewAppPlugin VERSION 1.0.0 LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Platform-specific Qt components
if(APPLE)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
elseif(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
else()
    # Linux: use QtWebView
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets WebView Quick)
endif()

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

# Platform-specific sources
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WebViewBridge.cpp
)

if(APPLE)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget_mac.mm)
elseif(WIN32)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget_win.cpp)
else()
    # Linux
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAppWidget_linux.cpp)
endif()

add_library(webview_app SHARED ${SOURCES})

# Add include directories before target_link_libraries
target_include_directories(webview_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# Platform-specific link libraries
if(APPLE)
    target_link_libraries(webview_app 
        PRIVATE 
            Qt6::Core 
            Qt6::Widgets
            "-framework WebKit"
            component-interfaces
    )
elseif(WIN32)
    target_link_libraries(webview_app 
        PRIVATE 
            Qt6::Core 
            Qt6::Widgets
            component-interfaces
    )
    # WebView2 is loaded dynamically, no static linking needed
else()
    # Linux
    target_link_libraries(webview_app 
        PRIVATE 
            Qt6::Core 
            Qt6::Widgets
            Qt6::WebView
            Qt6::Quick
            component-interfaces
    )
endif()

# For macOS, we need to set the bundle properties
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
