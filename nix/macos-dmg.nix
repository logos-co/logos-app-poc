# Creates a DMG disk image for LogosApp distribution
{ pkgs, appBundle }:

pkgs.stdenv.mkDerivation rec {
  pname = "LogosApp-dmg";
  version = "1.0.0";
  
  dontUnpack = true;
  __noChroot = pkgs.stdenv.isDarwin;
  
  nativeBuildInputs = [ ];
  
  buildPhase = ''
    runHook preBuild
    
    mkdir -p dmg_staging
    cp -RL "${appBundle}/LogosApp.app" dmg_staging/
    ln -s /Applications dmg_staging/Applications
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    mkdir -p $out
    
    HDIUTIL=/usr/bin/hdiutil
    APP_SIZE=$(du -sm dmg_staging | cut -f1)
    DMG_SIZE=$((APP_SIZE + 20))
    
    $HDIUTIL create -srcfolder dmg_staging \
      -volname "LogosApp" \
      -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" \
      -format UDRW \
      -size ''${DMG_SIZE}m \
      temp.dmg
    
    MOUNT_DIR=$(mktemp -d)
    $HDIUTIL attach temp.dmg -mountpoint "$MOUNT_DIR" -nobrowse -quiet
    $HDIUTIL detach "$MOUNT_DIR" -quiet
    rm -rf "$MOUNT_DIR"
    
    $HDIUTIL convert temp.dmg \
      -format UDZO \
      -imagekey zlib-level=9 \
      -o "$out/LogosApp-${version}.dmg"
    
    rm temp.dmg
    ln -s "LogosApp-${version}.dmg" "$out/LogosApp.dmg"
    
    runHook postInstall
  '';
  
  meta = with pkgs.lib; {
    description = "LogosApp macOS DMG Installer";
    platforms = platforms.darwin;
  };
}
