# The fixture host, minus main.cpp: shared by the desktop executable and the
# iOS stage in platform/ios/stage. Paths are relative to this file.
set(SHELL_PREVIEW_HOST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureBackend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureBackend.h
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureShellHost.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureShellHost.h
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureModels.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/FixtureModels.h
)
