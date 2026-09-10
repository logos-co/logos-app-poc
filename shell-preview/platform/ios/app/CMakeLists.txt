# The impure half: configured with the Xcode generator by
# `nix run .#run-ios-sim` against the store archives from the Ninja stages.
# Only main.cpp and the fixture resource are compiled here.
cmake_minimum_required(VERSION 3.24)
project(BasecampShellPreviewIos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick Qml QuickWidgets QuickControls2 Svg)
qt_standard_project_setup(REQUIRES 6.5)

find_package(LogosDesignSystem CONFIG REQUIRED)
find_package(BasecampMainUI CONFIG REQUIRED)
find_package(BasecampShellPreviewHost CONFIG REQUIRED)

# Directories qmlimportscanner walks to decide which static Qt QML plugins the
# bundle needs. The archives carry compiled QML, so the scan has to see sources.
set(BASECAMP_QML_SCAN_ROOTS "" CACHE STRING "QML source roots to scan for Qt plugin imports")
set(BASECAMP_FIXTURE ${CMAKE_CURRENT_SOURCE_DIR}/../../../fixtures/shell-fixture.json
    CACHE FILEPATH "Fixture JSON compiled into the bundle")

set(SHELL_PREVIEW_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src)

qt_add_executable(BasecampShellPreview MACOSX_BUNDLE ${SHELL_PREVIEW_SRC}/main.cpp)
target_compile_definitions(BasecampShellPreview PRIVATE SHELL_PREVIEW_STATIC_SHELL)

get_filename_component(fixture_dir ${BASECAMP_FIXTURE} DIRECTORY)
qt_add_resources(BasecampShellPreview "shell_preview_fixture"
    PREFIX "/shell-preview"
    BASE ${fixture_dir}
    FILES ${BASECAMP_FIXTURE}
)

# Qt6::Svg is linked here and not by the shell: it exists only to pull the
# static qsvg image format plugin into the bundle for the sidebar icons.
target_link_libraries(BasecampShellPreview PRIVATE
    Basecamp::ShellPreviewHost
    Basecamp::MainUI
    Logos::DesignSystem
    Qt6::Widgets
    Qt6::QuickWidgets
    Qt6::QuickControls2
    Qt6::Svg
)

set(QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS ${BASECAMP_QML_SCAN_ROOTS})

# Qt registers default plugins by globbing its own prefix's Qt6Gui/ directory;
# qsvg's config sits in the qtsvg prefix, so it is never seen. Blank icons.
include("${Qt6Svg_DIR}/../Qt6Gui/Qt6QSvgPluginConfig.cmake")
# The _init object carries the Q_IMPORT_PLUGIN; without it the archive is dead-stripped.
target_link_libraries(BasecampShellPreview PRIVATE Qt6::QSvgPlugin Qt6::QSvgPlugin_init)

set_target_properties(BasecampShellPreview PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER co.logos.basecamp.shellpreview
    MACOSX_BUNDLE_BUNDLE_NAME "Shell Preview"
    MACOSX_BUNDLE_BUNDLE_VERSION 1
    MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER co.logos.basecamp.shellpreview
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)
