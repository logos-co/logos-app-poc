cmake_minimum_required(VERSION 3.21)

project(Logos VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages (including RemoteObjects for liblogos)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 RemoteObjects)

qt_standard_project_setup()

# iOS-specific settings
if(IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0")
    
    # App bundle settings
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.qthelloworld")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
    set(MACOSX_BUNDLE_BUNDLE_NAME "Logos")
endif()

# liblogos integration
# LIBLOGOS_ROOT should point to the build-ios-sim or build-ios-device directory
set(LIBLOGOS_ROOT "" CACHE PATH "Path to prebuilt liblogos (e.g., build-ios-sim)")
set(LOGOS_CPP_SDK_ROOT "" CACHE PATH "Path to logos-cpp-sdk source")

# Plugin module paths
set(LOGOS_PACKAGE_MANAGER_ROOT "" CACHE PATH "Path to prebuilt package_manager (e.g., build-ios-sim)")
set(LOGOS_CAPABILITY_MODULE_ROOT "" CACHE PATH "Path to prebuilt capability_module (e.g., build-ios-sim)")

set(LIBLOGOS_FOUND FALSE)
set(PACKAGE_MANAGER_FOUND FALSE)
set(CAPABILITY_MODULE_FOUND FALSE)

if(LIBLOGOS_ROOT)
    message(STATUS "Looking for liblogos in: ${LIBLOGOS_ROOT}")
    
    # Try multiple locations for the libraries (direct file check, not find_library)
    set(_logos_lib_search_paths
        "${LIBLOGOS_ROOT}/install/lib"
        "${LIBLOGOS_ROOT}/lib/Debug"
        "${LIBLOGOS_ROOT}/lib"
    )
    
    foreach(_path ${_logos_lib_search_paths})
        if(EXISTS "${_path}/liblogos_core.a" AND EXISTS "${_path}/liblogos_sdk.a")
            set(LOGOS_CORE_LIBRARY "${_path}/liblogos_core.a")
            set(LOGOS_SDK_LIBRARY "${_path}/liblogos_sdk.a")
            message(STATUS "Found liblogos at: ${_path}")
            break()
        endif()
    endforeach()
    
    if(LOGOS_CORE_LIBRARY AND LOGOS_SDK_LIBRARY)
        message(STATUS "Found logos_core: ${LOGOS_CORE_LIBRARY}")
        message(STATUS "Found logos_sdk: ${LOGOS_SDK_LIBRARY}")
        
        # Create imported targets
        add_library(logos_core STATIC IMPORTED)
        set_target_properties(logos_core PROPERTIES
            IMPORTED_LOCATION "${LOGOS_CORE_LIBRARY}"
        )
        
        add_library(logos_sdk STATIC IMPORTED)
        set_target_properties(logos_sdk PROPERTIES
            IMPORTED_LOCATION "${LOGOS_SDK_LIBRARY}"
        )
        
        set(LIBLOGOS_FOUND TRUE)
    else()
        message(WARNING "liblogos libraries not found in ${LIBLOGOS_ROOT}")
    endif()
else()
    message(STATUS "LIBLOGOS_ROOT not set - building without liblogos integration")
endif()

# Find package_manager plugin
if(LOGOS_PACKAGE_MANAGER_ROOT)
    message(STATUS "Looking for package_manager in: ${LOGOS_PACKAGE_MANAGER_ROOT}")
    
    set(_pm_lib_search_paths
        "${LOGOS_PACKAGE_MANAGER_ROOT}/install/lib"
        "${LOGOS_PACKAGE_MANAGER_ROOT}/modules/Debug"
        "${LOGOS_PACKAGE_MANAGER_ROOT}/lib"
    )
    
    foreach(_path ${_pm_lib_search_paths})
        if(EXISTS "${_path}/package_manager_plugin.a")
            set(PACKAGE_MANAGER_LIBRARY "${_path}/package_manager_plugin.a")
            message(STATUS "Found package_manager at: ${_path}")
            break()
        endif()
    endforeach()
    
    if(PACKAGE_MANAGER_LIBRARY)
        message(STATUS "Found package_manager: ${PACKAGE_MANAGER_LIBRARY}")
        
        add_library(package_manager_plugin STATIC IMPORTED)
        set_target_properties(package_manager_plugin PROPERTIES
            IMPORTED_LOCATION "${PACKAGE_MANAGER_LIBRARY}"
        )
        
        set(PACKAGE_MANAGER_FOUND TRUE)
    else()
        message(WARNING "package_manager library not found in ${LOGOS_PACKAGE_MANAGER_ROOT}")
    endif()
else()
    message(STATUS "LOGOS_PACKAGE_MANAGER_ROOT not set - building without package_manager")
endif()

# Find capability_module plugin
if(LOGOS_CAPABILITY_MODULE_ROOT)
    message(STATUS "Looking for capability_module in: ${LOGOS_CAPABILITY_MODULE_ROOT}")
    
    set(_cm_lib_search_paths
        "${LOGOS_CAPABILITY_MODULE_ROOT}/install/lib"
        "${LOGOS_CAPABILITY_MODULE_ROOT}/modules/Debug"
        "${LOGOS_CAPABILITY_MODULE_ROOT}/lib"
    )
    
    foreach(_path ${_cm_lib_search_paths})
        if(EXISTS "${_path}/capability_module_plugin.a")
            set(CAPABILITY_MODULE_LIBRARY "${_path}/capability_module_plugin.a")
            message(STATUS "Found capability_module at: ${_path}")
            break()
        endif()
    endforeach()
    
    if(CAPABILITY_MODULE_LIBRARY)
        message(STATUS "Found capability_module: ${CAPABILITY_MODULE_LIBRARY}")
        
        add_library(capability_module_plugin STATIC IMPORTED)
        set_target_properties(capability_module_plugin PROPERTIES
            IMPORTED_LOCATION "${CAPABILITY_MODULE_LIBRARY}"
        )
        
        set(CAPABILITY_MODULE_FOUND TRUE)
    else()
        message(WARNING "capability_module library not found in ${LOGOS_CAPABILITY_MODULE_ROOT}")
    endif()
else()
    message(STATUS "LOGOS_CAPABILITY_MODULE_ROOT not set - building without capability_module")
endif()

set(GENERATED_CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_code")

# Create executable
qt_add_executable(Logos
    main.cpp
    resources.qrc
    ${GENERATED_CODE_DIR}/logos_sdk.cpp
)

# Add Assets.xcassets for iOS app icon
if(IOS)
    # Add asset catalog as a resource that will be compiled
    set_source_files_properties(Assets.xcassets PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(Logos PRIVATE Assets.xcassets)
endif()

# Base Qt libraries
target_link_libraries(Logos PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

# Link liblogos if found
if(LIBLOGOS_FOUND)
    message(STATUS "Enabling liblogos integration")
    target_compile_definitions(Logos PRIVATE LIBLOGOS_ENABLED=1)
    
    target_link_libraries(Logos PRIVATE
        logos_core
        logos_sdk
        Qt6::RemoteObjects
    )
    
    # Add include directories (support both install prefix and build directory layouts)
    if(EXISTS "${LIBLOGOS_ROOT}/include")
        target_include_directories(Logos PRIVATE
            "${LIBLOGOS_ROOT}/include"
        )
    elseif(EXISTS "${LIBLOGOS_ROOT}/install/include")
        target_include_directories(Logos PRIVATE
            "${LIBLOGOS_ROOT}/install/include"
        )
    endif()
    
    # Also include from SDK source if available (check both source and installed layouts)
    if(LOGOS_CPP_SDK_ROOT AND EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp")
        target_include_directories(Logos PRIVATE
            "${LOGOS_CPP_SDK_ROOT}/cpp"
        )
    elseif(LOGOS_CPP_SDK_ROOT AND EXISTS "${LOGOS_CPP_SDK_ROOT}/include/cpp")
        target_include_directories(Logos PRIVATE
            "${LOGOS_CPP_SDK_ROOT}/include/cpp"
        )
    endif()
    
    target_include_directories(Logos PRIVATE
        "${GENERATED_CODE_DIR}"
    )
endif()

# Link package_manager plugin if found
if(PACKAGE_MANAGER_FOUND)
    message(STATUS "Enabling package_manager plugin")
    target_compile_definitions(Logos PRIVATE PACKAGE_MANAGER_ENABLED=1)
    target_link_libraries(Logos PRIVATE package_manager_plugin)
endif()

# Link capability_module plugin if found
if(CAPABILITY_MODULE_FOUND)
    message(STATUS "Enabling capability_module plugin")
    target_compile_definitions(Logos PRIVATE CAPABILITY_MODULE_ENABLED=1)
    target_link_libraries(Logos PRIVATE capability_module_plugin)
endif()

# Set bundle properties
set_target_properties(Logos PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.qthelloworld"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# iOS-specific configuration
if(IOS)
    set_target_properties(Logos PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )
endif()
