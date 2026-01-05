# Builds a Linux AppImage for LogosApp
{ pkgs, app, version, src }:

assert pkgs.stdenv.isLinux;

let
  runtimeLibs = [
    pkgs.qt6.qtbase
    pkgs.qt6.qtremoteobjects
    pkgs.qt6.qtwebview
    pkgs.qt6.qtdeclarative
    pkgs.zstd
    pkgs.krb5
    pkgs.zlib
    pkgs.glib
    pkgs.freetype
    pkgs.fontconfig
    pkgs.libglvnd
    pkgs.mesa.drivers
    pkgs.xorg.libX11
    pkgs.xorg.libXext
    pkgs.xorg.libXrender
    pkgs.xorg.libXrandr
    pkgs.xorg.libXcursor
    pkgs.xorg.libXi
    pkgs.xorg.libXfixes
    pkgs.xorg.libxcb
  ];
  runtimeLibsStr = pkgs.lib.concatStringsSep " " (map toString runtimeLibs);
in
pkgs.stdenv.mkDerivation rec {
  pname = "logos-app-poc-appimage";
  inherit version;

  dontUnpack = true;
  nativeBuildInputs = [ pkgs.appimagekit ];
  buildInputs = runtimeLibs;

  installPhase = ''
    set -euo pipefail
    appDir=$out/LogosApp.AppDir
    mkdir -p "$appDir/usr"

    # Application payload
    cp -a ${app}/bin "$appDir/usr/"
    if [ -d ${app}/lib ]; then cp -a ${app}/lib "$appDir/usr/"; fi
    if [ -d ${app}/modules ]; then cp -a ${app}/modules "$appDir/usr/"; fi
    if [ -d ${app}/plugins ]; then cp -a ${app}/plugins "$appDir/usr/"; fi

    mkdir -p "$appDir/usr/lib"
    for dep in ${runtimeLibsStr}; do
      if [ -d "$dep/lib" ]; then
        cp -aL "$dep"/lib/*.so* "$appDir/usr/lib/" 2>/dev/null || true
        if [ -d "$dep/lib/qt-6" ]; then
          mkdir -p "$appDir/usr/lib/qt-6"
          cp -a "$dep/lib/qt-6/"* "$appDir/usr/lib/qt-6/" 2>/dev/null || true
        fi
      fi
    done

    # Qt plugins and QML modules
    mkdir -p "$appDir/usr/lib/qt-6/plugins" "$appDir/usr/lib/qt-6/qml"
    cp -a ${pkgs.qt6.qtbase}/lib/qt-6/plugins/* "$appDir/usr/lib/qt-6/plugins/" 2>/dev/null || true
    cp -a ${pkgs.qt6.qtwebview}/lib/qt-6/plugins/* "$appDir/usr/lib/qt-6/plugins/" 2>/dev/null || true
    cp -a ${pkgs.qt6.qtdeclarative}/lib/qt-6/qml/* "$appDir/usr/lib/qt-6/qml/" 2>/dev/null || true
    cp -a ${pkgs.qt6.qtwebview}/lib/qt-6/qml/* "$appDir/usr/lib/qt-6/qml/" 2>/dev/null || true

    # Desktop entry and icon
    mkdir -p "$appDir/usr/share/applications" "$appDir/usr/share/icons/hicolor/256x256/apps"
    cat > "$appDir/usr/share/applications/logos-app.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Logos App POC
Exec=LogosApp
Icon=logos-app
Categories=Utility;
EOF
    cp ${src}/app/icons/logos.png "$appDir/usr/share/icons/hicolor/256x256/apps/logos-app.png"
    ln -sf usr/share/icons/hicolor/256x256/apps/logos-app.png "$appDir/.DirIcon"

    # AppRun launcher to wire up runtime paths
    cat > "$appDir/AppRun" <<'EOF'
#!/bin/sh
APPDIR="$(dirname "$(readlink -f "$0")")"
export PATH="$APPDIR/usr/bin:$PATH"
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$APPDIR/usr/lib/qt-6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QML2_IMPORT_PATH="$APPDIR/usr/lib/qt-6/qml${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
export NIXPKGS_QT6_QML_IMPORT_PATH="$QML2_IMPORT_PATH"
exec "$APPDIR/usr/bin/LogosApp" "$@"
EOF
    chmod +x "$appDir/AppRun"

    # Build the AppImage payload
    ${pkgs.appimagekit}/bin/appimagetool "$appDir" "$out/LogosApp-${version}.AppImage"
  '';

  meta = with pkgs.lib; {
    description = "Logos App POC AppImage";
    platforms = platforms.linux;
    mainProgram = "LogosApp";
  };
}
