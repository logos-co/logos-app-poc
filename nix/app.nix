# Builds the logos-app-poc standalone application
{ pkgs, common, src, logosLiblogos, logosSdk, logosPackageManager, logosCapabilityModule, counterPlugin, mainUIPlugin }:

pkgs.stdenv.mkDerivation rec {
  pname = "logos-app-poc-app";
  version = common.version;
  
  inherit src;
  inherit (common) buildInputs meta;
  
  # Add logosSdk to nativeBuildInputs for logos-cpp-generator
  nativeBuildInputs = common.nativeBuildInputs ++ [ logosSdk ];
  
  preConfigure = ''
    runHook prePreConfigure
    
    # Copy logos-cpp-sdk headers to expected location
    echo "Copying logos-cpp-sdk headers for app..."
    mkdir -p ./logos-cpp-sdk/include/cpp
    cp -r ${logosSdk}/include/cpp/* ./logos-cpp-sdk/include/cpp/
    
    # Also copy core headers
    echo "Copying core headers..."
    mkdir -p ./logos-cpp-sdk/include/core
    cp -r ${logosSdk}/include/core/* ./logos-cpp-sdk/include/core/
    
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
    
    runHook postPreConfigure
  '';
  
  # This is a GUI application, enable Qt wrapping
  dontWrapQtApps = false;
  
  # This is an aggregate runtime layout; avoid stripping to prevent hook errors
  dontStrip = true;
  
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring logos-app-poc-app..."
    echo "liblogos: ${logosLiblogos}"
    echo "cpp-sdk: ${logosSdk}"
    echo "package-manager: ${logosPackageManager}"
    echo "capability-module: ${logosCapabilityModule}"
    echo "counter-plugin: ${counterPlugin}"
    echo "main-ui-plugin: ${mainUIPlugin}"
    
    # Verify that the built components exist
    test -d "${logosLiblogos}" || (echo "liblogos not found" && exit 1)
    test -d "${logosSdk}" || (echo "cpp-sdk not found" && exit 1)
    test -d "${logosPackageManager}" || (echo "package-manager not found" && exit 1)
    test -d "${logosCapabilityModule}" || (echo "capability-module not found" && exit 1)
    test -d "${counterPlugin}" || (echo "counter-plugin not found" && exit 1)
    test -d "${mainUIPlugin}" || (echo "main-ui-plugin not found" && exit 1)
    
    cmake -S app -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DLOGOS_LIBLOGOS_ROOT=${logosLiblogos} \
      -DLOGOS_CPP_SDK_ROOT=$(pwd)/logos-cpp-sdk
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "logos-app-poc-app built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    # Create output directories
    mkdir -p $out/bin $out/lib $out/modules $out/plugins
    
    # Install our app binary
    if [ -f "build/LogosApp" ]; then
      cp build/LogosApp "$out/bin/"
      echo "Installed LogosApp binary"
    fi
    
    # Copy the core binaries from liblogos
    if [ -f "${logosLiblogos}/bin/logoscore" ]; then
      cp -L "${logosLiblogos}/bin/logoscore" "$out/bin/"
      echo "Installed logoscore binary"
    fi
    if [ -f "${logosLiblogos}/bin/logos_host" ]; then
      cp -L "${logosLiblogos}/bin/logos_host" "$out/bin/"
      echo "Installed logos_host binary"
    fi
    
    # Copy required shared libraries from liblogos
    if ls "${logosLiblogos}/lib/"liblogos_core.* >/dev/null 2>&1; then
      cp -L "${logosLiblogos}/lib/"liblogos_core.* "$out/lib/" || true
    fi
    
    # Copy SDK library if it exists
    if ls "${logosSdk}/lib/"liblogos_sdk.* >/dev/null 2>&1; then
      cp -L "${logosSdk}/lib/"liblogos_sdk.* "$out/lib/" || true
    fi

    # Determine platform-specific plugin extension
    OS_EXT="so"
    case "$(uname -s)" in
      Darwin) OS_EXT="dylib";;
      Linux) OS_EXT="so";;
      MINGW*|MSYS*|CYGWIN*) OS_EXT="dll";;
    esac

    # Copy module plugins into the modules directory
    if [ -f "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" ]; then
      cp -L "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" "$out/modules/"
    fi
    if [ -f "${logosPackageManager}/lib/package_manager_plugin.$OS_EXT" ]; then
      cp -L "${logosPackageManager}/lib/package_manager_plugin.$OS_EXT" "$out/modules/"
    fi
    
    # Copy UI plugins to plugins directory (separate from liblogos modules)
    if [ -f "${counterPlugin}/lib/counter.$OS_EXT" ]; then
      cp -L "${counterPlugin}/lib/counter.$OS_EXT" "$out/plugins/"
      echo "Copied counter.$OS_EXT to plugins/"
    fi
    if [ -f "${mainUIPlugin}/lib/main_ui.$OS_EXT" ]; then
      cp -L "${mainUIPlugin}/lib/main_ui.$OS_EXT" "$out/plugins/"
      echo "Copied main_ui.$OS_EXT to plugins/"
    fi

    # Create a README for reference
    cat > $out/README.txt <<EOF
Logos App POC - Build Information
==================================
liblogos: ${logosLiblogos}
cpp-sdk: ${logosSdk}
package-manager: ${logosPackageManager}
capability-module: ${logosCapabilityModule}
counter-plugin: ${counterPlugin}
main-ui-plugin: ${mainUIPlugin}

Runtime Layout:
- Binary: $out/bin/LogosApp
- Libraries: $out/lib
- Modules: $out/modules

Usage:
  $out/bin/LogosApp
EOF
    
    runHook postInstall
  '';
}
