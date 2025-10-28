# Builds the counter plugin
{ pkgs, common, src }:

pkgs.stdenv.mkDerivation {
  pname = "${common.pname}-counter-plugin";
  version = common.version;
  
  inherit src;
  inherit (common) nativeBuildInputs buildInputs meta;
  
  # Simple CMake flags for counter plugin
  cmakeFlags = [
    "-GNinja"
    "-DCMAKE_BUILD_TYPE=Release"
  ];
  
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring counter plugin..."
    cmake -S logos_dapps/counter -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "Counter plugin built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    mkdir -p $out/lib
    
    # Find and copy the built library file
    if [ -f "build/counter.dylib" ]; then
      cp build/counter.dylib $out/lib/
    elif [ -f "build/counter.so" ]; then
      cp build/counter.so $out/lib/
    else
      echo "Error: No counter library file found"
      exit 1
    fi
    
    runHook postInstall
  '';
}
