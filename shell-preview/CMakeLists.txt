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
if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS Quick QuickWidgets QuickControls2 Svg)
endif()

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

if(ANDROID)
    # The Shell plugin is built separately (nix/main-ui.nix); androiddeployqt
    # ships it as an extra library so the host finds it in the APK's lib dir.
    set(SHELL_PREVIEW_ANDROID_SHELL_PLUGIN "" CACHE FILEPATH "libmain_ui.so to bundle")
    if(NOT SHELL_PREVIEW_ANDROID_SHELL_PLUGIN)
        message(FATAL_ERROR "SHELL_PREVIEW_ANDROID_SHELL_PLUGIN must point at libmain_ui.so")
    endif()
    # androiddeployqt cannot see into libmain_ui.so: the host links what the
    # Shell links and the Shell's QML sources are scanned for imports.
    set(SHELL_PREVIEW_ANDROID_QML_ROOTS "" CACHE STRING "Extra QML roots to scan for imports")
    target_link_libraries(basecamp-shell-preview PRIVATE
        Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 Qt6::Svg)
    # Qt6Gui only registers plugin packages from its own prefix (nixpkgs splits
    # qtsvg off), so the SVG image format is put on the deploy list by hand.
    find_package(Qt6QSvgPlugin CONFIG REQUIRED PATHS "${Qt6Svg_DIR}/../Qt6Gui" NO_DEFAULT_PATH)
    set_property(TARGET basecamp-shell-preview APPEND PROPERTY QT_PLUGIN_TARGETS Qt6::QSvgPlugin)
    set(qml_roots "${CMAKE_CURRENT_SOURCE_DIR}/../src/Basecamp")
    list(APPEND qml_roots ${SHELL_PREVIEW_ANDROID_QML_ROOTS})
    set_target_properties(basecamp-shell-preview PROPERTIES
        QT_ANDROID_PACKAGE_NAME "co.logos.basecamp.shellpreview"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/android"
        QT_ANDROID_EXTRA_LIBS "${SHELL_PREVIEW_ANDROID_SHELL_PLUGIN}"
        QT_QML_ROOT_PATH "${qml_roots}"
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_VERSION_NAME "0.1"
    )
endif()
