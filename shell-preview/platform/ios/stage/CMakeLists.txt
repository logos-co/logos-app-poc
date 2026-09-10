# iOS stage of shell-preview: the fixture host as a static archive, built with
# Ninja inside nix. Everything except main.cpp, which the Xcode project in
# ../app compiles so the bundle has a real entry point of its own.
cmake_minimum_required(VERSION 3.24)
project(BasecampShellPreviewHost LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml)

set(SHELL_PREVIEW_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../../src)
set(BASECAMP_INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/../../../../app/interfaces)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../sources.cmake)

add_library(shell_preview_host STATIC ${SHELL_PREVIEW_HOST_SOURCES})

target_include_directories(shell_preview_host PUBLIC
    $<BUILD_INTERFACE:${SHELL_PREVIEW_SRC}>
    $<BUILD_INTERFACE:${BASECAMP_INTERFACES}>
    $<INSTALL_INTERFACE:include/basecamp-shell-preview>
)

target_link_libraries(shell_preview_host PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml)

set_target_properties(shell_preview_host PROPERTIES EXPORT_NAME ShellPreviewHost)

install(TARGETS shell_preview_host EXPORT BasecampShellPreviewHostTargets
    ARCHIVE DESTINATION lib)
install(FILES
    ${SHELL_PREVIEW_SRC}/FixtureBackend.h
    ${SHELL_PREVIEW_SRC}/FixtureShellHost.h
    ${SHELL_PREVIEW_SRC}/FixtureModels.h
    ${BASECAMP_INTERFACES}/IShellHost.h
    ${BASECAMP_INTERFACES}/IShellView.h
    ${BASECAMP_INTERFACES}/BasecampModelRoles.h
    ${BASECAMP_INTERFACES}/InstallEnums.h
    ${BASECAMP_INTERFACES}/ShellSections.h
    DESTINATION include/basecamp-shell-preview)
install(EXPORT BasecampShellPreviewHostTargets
    FILE BasecampShellPreviewHostTargets.cmake
    NAMESPACE Basecamp::
    DESTINATION lib/cmake/BasecampShellPreviewHost)
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/BasecampShellPreviewHostConfig.cmake CONTENT [[
include(CMakeFindDependencyMacro)
find_dependency(Qt6 COMPONENTS Core Gui Widgets Qml)
include("${CMAKE_CURRENT_LIST_DIR}/BasecampShellPreviewHostTargets.cmake")
]] @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BasecampShellPreviewHostConfig.cmake
    DESTINATION lib/cmake/BasecampShellPreviewHost)
