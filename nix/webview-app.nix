# Builds the webview_app plugin
{ pkgs, common, src }:

pkgs.stdenv.mkDerivation {
  pname = "${common.pname}-webview-app-plugin";
  version = common.version;
  
  inherit src;
  nativeBuildInputs = common.nativeBuildInputs;
  
  # QtWebView and QuickWidgets needed on all platforms
  buildInputs = common.buildInputs ++ [
    pkgs.qt6.qtwebview
    pkgs.qt6.qtdeclarative  # Provides Qt Quick
  ] ++ (
    if pkgs.stdenv.isLinux then
      # Linux also needs WebKitGTK as the backend for QtWebView
      [ pkgs.webkitgtk ]
    else
      []
  );
  
  inherit (common) meta;
  
  # Simple CMake flags for webview_app plugin
  cmakeFlags = [
    "-GNinja"
    "-DCMAKE_BUILD_TYPE=Release"
  ];
  
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring webview_app plugin..."
    cmake -S logos_dapps/webview_app -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "WebView app plugin built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    mkdir -p $out/lib
    
    # Find and copy the built library file
    # QML and HTML files are now embedded in the library via qrc
    if [ -f "build/webview_app.dylib" ]; then
      cp build/webview_app.dylib $out/lib/
    elif [ -f "build/webview_app.so" ]; then
      cp build/webview_app.so $out/lib/
    else
      echo "Error: No webview_app library file found"
      exit 1
    fi
    
    runHook postInstall
  '';
}
