# shell-preview — the Basecamp UI shell against a fixture, with no Logos code.
#
# Depends on Qt and app/interfaces only. Deliberately does NOT link liblogos,
# logos-protocol, logos-qt-host or logos-view-module-runtime; the whole point is
# that a core rework cannot reach this binary.
#
# It loads the real, shipped main_ui.so at runtime — the same artifact the real
# host loads, unmodified.

cmake_minimum_required(VERSION 3.21)
project(BasecampShellPreview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qml only for ShellSections.h (QML_ELEMENT), which the fixture validates against.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml)

include(sources.cmake)

qt_add_executable(basecamp-shell-preview src/main.cpp ${SHELL_PREVIEW_HOST_SOURCES})

qt_add_resources(basecamp-shell-preview "shell_preview_fixture"
    PREFIX "/shell-preview"
    BASE "fixtures"
    FILES fixtures/shell-fixture.json
)

target_include_directories(basecamp-shell-preview PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # IShellHost.h, IShellView.h, BasecampModelRoles.h — the boundary the shell
    # is compiled against. Headers only; nothing is linked from the app.
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/interfaces
)

target_link_libraries(basecamp-shell-preview PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml)

# Link a MAIN_UI_STATIC build of the shell into the host instead of loading
# main_ui.so at runtime; needs the BasecampMainUI package that build exports.
option(SHELL_PREVIEW_STATIC_SHELL "Link main_ui statically into the host" OFF)
if(SHELL_PREVIEW_STATIC_SHELL)
    find_package(Qt6 REQUIRED COMPONENTS Quick QuickWidgets QuickControls2)
    find_package(LogosDesignSystem CONFIG REQUIRED)
    find_package(BasecampMainUI CONFIG REQUIRED)
    target_compile_definitions(basecamp-shell-preview PRIVATE SHELL_PREVIEW_STATIC_SHELL)
    target_link_libraries(basecamp-shell-preview PRIVATE Basecamp::MainUI Logos::DesignSystem
        Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2)
endif()
