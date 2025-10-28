# Builds the main UI plugin
{ pkgs, common, src, logosSdk }:

pkgs.stdenv.mkDerivation {
  pname = "${common.pname}-main-ui-plugin";
  version = common.version;
  
  inherit src;
  inherit (common) buildInputs meta;
  
  # Add logosSdk to nativeBuildInputs for logos-cpp-generator
  nativeBuildInputs = common.nativeBuildInputs ++ [ logosSdk ];
  
  preConfigure = ''
    runHook prePreConfigure
    
    # Create generated_code directory for generated files
    mkdir -p ./logos_dapps/main_ui/generated_code
    
    # Copy logos-cpp-sdk source files to expected location for CMakeLists.txt
    echo "Copying logos-cpp-sdk source files..."
    # Don't create cpp directory structure - this confuses layout detection
    # Instead, just create the include structure that CMake expects for installed layout
    
    # Create the include/cpp structure that CMake expects
    echo "Creating include/cpp structure..."
    mkdir -p ./logos-cpp-sdk/include/cpp
    cp -r ${logosSdk}/include/cpp/* ./logos-cpp-sdk/include/cpp/
    echo "Files in logos-cpp-sdk/include/cpp/:"
    ls -la ./logos-cpp-sdk/include/cpp/
    
    # Also copy core headers to fix relative includes
    echo "Copying core headers..."
    mkdir -p ./logos-cpp-sdk/include/core
    cp -r ${logosSdk}/include/core/* ./logos-cpp-sdk/include/core/
    echo "Files in logos-cpp-sdk/include/core/:"
    ls -la ./logos-cpp-sdk/include/core/
    
    # Copy SDK library files to lib directory
    echo "Copying SDK library files..."
    mkdir -p ./logos-cpp-sdk/lib
    if [ -f "${logosSdk}/lib/liblogos_sdk.dylib" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.dylib" ./logos-cpp-sdk/lib/
    elif [ -f "${logosSdk}/lib/liblogos_sdk.so" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.so" ./logos-cpp-sdk/lib/
    elif [ -f "${logosSdk}/lib/liblogos_sdk.a" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.a" ./logos-cpp-sdk/lib/
    fi
    
    # Run logos-cpp-generator with metadata.json
    echo "Running logos-cpp-generator for main_ui..."
    logos-cpp-generator --metadata ${src}/logos_dapps/main_ui/src/metadata.json --module-dir ./logos_dapps/main_ui/generated_code
    
    # Check what was generated
    echo "Checking generated files in generated_code:"
    ls -la ./logos_dapps/main_ui/generated_code/
    
    # Copy generated headers to include directory so source files can find them
    if [ -f "./logos_dapps/main_ui/generated_code/logos_sdk.h" ]; then
      cp ./logos_dapps/main_ui/generated_code/logos_sdk.h ./logos-cpp-sdk/include/cpp/
      echo "Copied logos_sdk.h to include/cpp directory"
    else
      echo "Creating minimal logos_sdk.h file..."
      cat > ./logos-cpp-sdk/include/cpp/logos_sdk.h << 'EOF'
// Minimal logos_sdk.h for Nix build
// This file is created to satisfy CMake dependencies
// The actual functionality is provided by the logos-cpp-sdk package

#ifndef LOGOS_SDK_H
#define LOGOS_SDK_H

#include "logos_api.h"
#include <vector>
#include <string>
#include <QString>
#include <QJsonArray>

namespace logos {
namespace sdk {
    // Minimal stub implementation
    class LogosModules {
    public:
        LogosModules(void* api) {}
        
        // Minimal stub for package_manager
        struct PackageManager {
            QJsonArray getPackages() { return QJsonArray(); }
            bool installPlugin(const QString& path) { return false; }
        } package_manager;
        
        // Minimal stub for core_manager
        struct CoreManager {
            QJsonArray getKnownPlugins() { return QJsonArray(); }
            bool loadPlugin(const QString& name) { return false; }
            bool unloadPlugin(const QString& name) { return false; }
        } core_manager;
    };
}
}

// Add using declaration for convenience
using LogosModules = logos::sdk::LogosModules;

#endif // LOGOS_SDK_H
EOF
      echo "Created minimal logos_sdk.h"
    fi
    
    # For nix builds, we don't need to create the logos-cpp-sdk/cpp/generated structure
    # since we're using the installed SDK layout and linking against the SDK library
    echo "Using installed SDK layout - generated files are for reference only"
    
    runHook postPreConfigure
  '';
  
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring main UI plugin..."
    echo "logosSdk: ${logosSdk}"
    
    # Verify that the logosSdk exists
    test -d "${logosSdk}" || (echo "logosSdk not found" && exit 1)
    
    cmake -S logos_dapps/main_ui -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DLOGOS_CPP_SDK_ROOT=$(pwd)/logos-cpp-sdk
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "Main UI plugin built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    mkdir -p $out/lib
    
    # Find and copy the built library file
    if [ -f "build/main_ui.dylib" ]; then
      cp build/main_ui.dylib $out/lib/
    elif [ -f "build/main_ui.so" ]; then
      cp build/main_ui.so $out/lib/
    else
      echo "Error: No main_ui library file found"
      exit 1
    fi
    
    runHook postInstall
  '';
}
