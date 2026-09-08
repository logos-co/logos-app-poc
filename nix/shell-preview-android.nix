# The Shell preview as a debug-signed APK: the shell-preview host packaged by
# pkgs.mkQtAndroidApk, with the cross-built Shell plugin shipped as an extra
# library that the host QPluginLoader()s from the APK's lib dir.
#
# Same rule as nix/shell-preview.nix: no logos input, ever. mainUIPlugin is a
# RUNTIME input only, and the APK is checked for Logos libraries after gradle.
{ pkgs, common, src, mainUIPlugin, logosDesignSystem }:

let
  inherit (pkgs) lib;
  packageName = "co.logos.basecamp.shellpreview";
  activity = "org.qtproject.qt.android.bindings.QtActivity";
  qtModules = with pkgs.qt6; [ qtbase qtdeclarative qtsvg ];

  apk = (pkgs.mkQtAndroidApk {
    pname = "${common.pname}-shell-preview";
    version = common.version;
    inherit src packageName qtModules;
    target = "basecamp-shell-preview";
    nativeBuildInputs = [ pkgs.buildPackages.unzip ];
    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=Release"
      "-DSHELL_PREVIEW_ANDROID_SHELL_PLUGIN=${mainUIPlugin}/plugins/main_ui/libmain_ui.so"
      # The design system's QML is compiled into libmain_ui.so; its imports
      # (QtQuick.Effects, QtCore, ...) are only visible in its sources.
      "-DSHELL_PREVIEW_ANDROID_QML_ROOTS=${logosDesignSystem.src}/src"
    ];
    meta.description = "Logos Basecamp Shell preview, packaged as an Android APK";
  }).overrideAttrs (old: {
    # shell-preview/ is the CMake project, but it includes ../app/interfaces,
    # so the whole tree is the source and only the configure root moves.
    setSourceRoot = "sourceRoot=$(echo */shell-preview)";

    # The DT_NEEDED gate in mkQtAndroidApk cannot catch a Logos library: it
    # would be packaged, not missing.
    postInstall = (old.postInstall or "") + ''
      libs=$(unzip -Z1 "$out/${old.passthru.apkName}" | grep '^lib/' || true)
      if printf '%s\n' "$libs" | grep -E 'liblogos|logos_protocol'; then
        echo "shell-preview-android: a Logos library is packaged in the APK" >&2
        exit 1
      fi
      printf '%s\n' "$libs" | grep -qx 'lib/${old.passthru.abi}/libmain_ui.so' || {
        echo "shell-preview-android: libmain_ui.so is not in the APK" >&2
        exit 1
      }
    '';
  });

  apkFile = "${apk}/${apk.apkName}";
  adb = "${pkgs.androidPkgs.androidsdk}/bin/adb";

  # Installs on the one attached device, or the one named by --device /
  # ANDROID_SERIAL, and launches the activity.
  runner = pkgs.buildPackages.writeShellScriptBin "run-android" ''
    set -euo pipefail
    adb=${adb}
    apk=${apkFile}
    pkg=${packageName}

    serial="''${ANDROID_SERIAL:-}"
    while [ $# -gt 0 ]; do
      case "$1" in
        --device|-d) serial="$2"; shift 2 ;;
        --device=*) serial="''${1#--device=}"; shift ;;
        -h|--help)
          echo "usage: run-android [--device <serial>]   (or ANDROID_SERIAL=<serial>)"
          exit 0 ;;
        *) echo "run-android: unknown argument: $1" >&2; exit 2 ;;
      esac
    done

    if [ -z "$serial" ]; then
      devices=$("$adb" devices | awk 'NR > 1 && $2 == "device" { print $1 }')
      count=$(printf '%s\n' "$devices" | grep -c . || true)
      if [ "$count" -eq 0 ]; then
        echo "run-android: no device in state 'device' (adb devices)" >&2
        exit 1
      elif [ "$count" -gt 1 ]; then
        echo "run-android: several devices attached; pick one with --device <serial> or ANDROID_SERIAL:" >&2
        printf '  %s\n' $devices >&2
        exit 1
      fi
      serial=$devices
    fi
    export ANDROID_SERIAL="$serial"

    echo "run-android: installing $apk on $serial"
    if ! out=$("$adb" install -r "$apk" 2>&1); then
      # A previous install signed with another key: only an uninstall clears it.
      if printf '%s' "$out" | grep -q INSTALL_FAILED_UPDATE_INCOMPATIBLE; then
        echo "run-android: signer mismatch, uninstalling $pkg first"
        "$adb" uninstall "$pkg" >/dev/null
        "$adb" install -r "$apk"
      else
        printf '%s\n' "$out" >&2
        exit 1
      fi
    fi

    "$adb" shell am start -W -n "$pkg/${activity}" >/dev/null
    pid=$("$adb" shell pidof "$pkg" | tr -d '\r')
    echo "run-android: $pkg on $serial, pid $pid"
  '';
in
# passthru only: the derivation is `apk` unchanged.
apk.overrideAttrs (old: {
  passthru = old.passthru // { inherit packageName activity runner; };
})
