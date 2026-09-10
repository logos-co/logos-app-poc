# shell-preview for iOS: the pure half as static archives (pkgs.mkIosCmakeStage),
# plus run-ios-sim / run-ios-device, the impure Xcode-generator link and
# simctl / devicectl step for whichever SDK this package set targets.
{ pkgs, src, version, designSystemSrc }:

let
  inherit (pkgs) lib;
  buildPkgs = pkgs.pkgsBuildBuild;
  appleSdk = pkgs.qt6.qtbase.appleSdk;

  designSystem = pkgs.mkIosCmakeStage {
    pname = "logos-design-system-ios";
    version = "1.0.0";
    src = designSystemSrc;
  };

  mainUi = pkgs.mkIosCmakeStage {
    pname = "logos-basecamp-main-ui-ios";
    inherit version src;
    sourceDir = "src";
    buildInputs = [ designSystem ];
    cmakeFlags = [
      "-DMAIN_UI_STATIC=ON"
      "-DLogosDesignSystem_DIR=${designSystem}/lib/cmake/LogosDesignSystem"
    ];
  };

  stage = pkgs.mkIosCmakeStage {
    pname = "logos-basecamp-shell-preview-ios";
    inherit version src;
    sourceDir = "shell-preview/platform/ios/stage";
  };

  # Shared by both runners: the version gate, the Xcode-generator configure
  # against the store archives and the xcodebuild step. Callers pass the
  # signing choice: `configure_app [-D...]` and `xcodebuild_app [flags...]`.
  buildApp = ''
    # The stages were gated at build time; the link step runs later, so
    # gate again before touching xcodebuild.
    ${pkgs.xcodeWrapper.versionGate}

    app_src=${src}/shell-preview/platform/ios/app
    bundle_id=co.logos.basecamp.shellpreview
    # keyed on the stage store path: CMake refuses a cache made from other inputs
    build_dir="''${LOGOS_IOS_SHELL_PREVIEW_BUILD_DIR:-''${TMPDIR:-/tmp}/basecamp-shell-preview-ios/$(basename ${stage})}"
    app="$build_dir/Debug-${appleSdk}/BasecampShellPreview.app"

    configure_app() {
      mkdir -p "$build_dir"
      # CMAKE_FIND_ROOT_PATH too: an iOS sysroot puts find_package in
      # root-only mode, where CMAKE_PREFIX_PATH alone finds nothing.
      echo "==> configure ($build_dir)"
      cmake -S "$app_src" -B "$build_dir" -G Xcode \
        -DCMAKE_TOOLCHAIN_FILE=${pkgs.logosQtCrossToolchainFile} \
        ${lib.escapeShellArgs pkgs.logosQtCrossCmakeFlags} \
        "-DCMAKE_PREFIX_PATH=${stage};${mainUi};${designSystem}" \
        "-DCMAKE_FIND_ROOT_PATH=${stage};${mainUi};${designSystem}" \
        "-DBASECAMP_QML_SCAN_ROOTS=${src}/src/Basecamp;${designSystemSrc}/src/qml" \
        "-DBASECAMP_FIXTURE=${src}/shell-preview/fixtures/shell-fixture.json" \
        "$@"
    }

    xcodebuild_app() {
      echo "==> xcodebuild (${appleSdk}; full log: $build_dir/xcodebuild.log)"
      rm -rf "$app"
      # errexit+pipefail would exit on grep's status before the message below
      set +e
      xcodebuild -project "$build_dir/BasecampShellPreviewIos.xcodeproj" -target BasecampShellPreview \
        -configuration Debug -sdk ${appleSdk} -arch arm64 "$@" \
        build 2>&1 | tee "$build_dir/xcodebuild.log" | grep -E '^\*\*|error:|warning: .*ld'
      xcode_status=''${PIPESTATUS[0]}
      set -e
      if [ "$xcode_status" -ne 0 ]; then
        echo "xcodebuild failed; see $build_dir/xcodebuild.log" >&2
        exit 1
      fi
      [ -d "$app" ] || { echo "xcodebuild produced no $app" >&2; exit 1; }
    }

    # simctl and devicectl return 0 whatever the app did, so an attached app
    # that is gone within seconds is treated as the startup qFatal it almost
    # always is.
    launched_at=0
    mark_launch() { launched_at=$(date +%s); }
    check_launch() {
      if [ $(( $(date +%s) - launched_at )) -lt 5 ]; then
        echo "app exited right after launch; the console output above says why" >&2
        exit 1
      fi
    }
  '';

  runIosSim = buildPkgs.writeShellApplication {
    name = "run-ios-sim";
    runtimeInputs = [ buildPkgs.cmake pkgs.xcodeWrapper ];
    text = ''
      ${buildApp}
      configure_app -DBASECAMP_IOS_DEVELOPMENT_TEAM=
      xcodebuild_app CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY=

      udid=$(xcrun simctl list devices booted | grep -o -m1 '[0-9A-F-]\{36\}' || true)
      if [ -z "$udid" ]; then
        udid=$(xcrun simctl list devices available | grep -m1 'iPhone' | grep -o '[0-9A-F-]\{36\}')
        echo "==> no simulator booted; booting $udid"
        xcrun simctl boot "$udid"
      fi
      open -a Simulator --args -CurrentDeviceUDID "$udid"
      xcrun simctl bootstatus "$udid" -b >/dev/null

      echo "==> install + launch on $udid (console attached; extra arguments reach the app)"
      echo "    more: xcrun simctl spawn $udid log stream --level info --predicate 'process == \"BasecampShellPreview\"'"
      xcrun simctl install "$udid" "$app"
      mark_launch
      xcrun simctl launch --console-pty "$udid" "$bundle_id" "$@"
      check_launch
    '';
  };

  # Automatic signing for the team in LOGOS_IOS_TEAM_ID, then devicectl on the
  # one available device (--device / LOGOS_IOS_DEVICE when several are).
  runIosDevice = buildPkgs.writeShellApplication {
    name = "run-ios-device";
    runtimeInputs = [ buildPkgs.cmake pkgs.xcodeWrapper ];
    text = ''
      device="''${LOGOS_IOS_DEVICE:-}"
      while [ $# -gt 0 ]; do
        case "$1" in
          --device|-d)
            [ $# -ge 2 ] || { echo "run-ios-device: --device needs a udid" >&2; exit 1; }
            device="$2"; shift 2 ;;
          --device=*) device="''${1#--device=}"; shift ;;
          -h|--help)
            echo "usage: run-ios-device [--device <udid>] [app arguments...]"
            echo "  needs LOGOS_IOS_TEAM_ID=<apple team id>; LOGOS_IOS_DEVICE=<udid> is the same as --device"
            exit 0 ;;
          *) break ;;
        esac
      done

      team="''${LOGOS_IOS_TEAM_ID:-}"
      if [ -z "$team" ]; then
        echo "run-ios-device: LOGOS_IOS_TEAM_ID is unset; export the Apple development team id (Xcode > Settings > Accounts, e.g. LOGOS_IOS_TEAM_ID=ABCDE12345)" >&2
        exit 1
      fi

      # devicectl installs to `available (paired)` and to `connected` (unlocked, active).
      available=$(xcrun devicectl list devices --hide-headers 2>/dev/null \
        | grep -oE '[0-9A-F-]{36} +(available \(paired\)|connected)' | cut -d' ' -f1 || true)
      if [ -z "$device" ]; then
        count=$(printf '%s\n' "$available" | grep -c . || true)
        if [ "$count" -eq 0 ]; then
          echo "run-ios-device: no available device; connect and pair one, then see xcrun devicectl list devices" >&2
          exit 1
        elif [ "$count" -gt 1 ]; then
          echo "run-ios-device: several devices available; pick one with --device <udid> or LOGOS_IOS_DEVICE (xcrun devicectl list devices):" >&2
          xcrun devicectl list devices --hide-headers --filter "State BEGINSWITH 'available' OR State == 'connected'" >&2 || true
          exit 1
        fi
        device=$available
      elif ! printf '%s\n' "$available" | grep -qxF "$device"; then
        echo "run-ios-device: $device is not an available device (xcrun devicectl list devices)" >&2
        exit 1
      fi

      ${buildApp}
      configure_app "-DBASECAMP_IOS_DEVELOPMENT_TEAM=$team"
      xcodebuild_app -allowProvisioningUpdates

      echo "==> install + launch on $device (console attached; extra arguments reach the app)"
      xcrun devicectl device install app --device "$device" "$app"
      mark_launch
      xcrun devicectl device process launch --activate --console --terminate-existing \
        --device "$device" "$bundle_id" "$@"
      check_launch
    '';
  };
in
{
  design-system = designSystem;
  main-ui-plugin = mainUi;
  shell-preview-ios = stage;
}
// lib.optionalAttrs (appleSdk == "iphonesimulator") {
  run-ios-sim = runIosSim;
}
// lib.optionalAttrs (appleSdk == "iphoneos") {
  run-ios-device = runIosDevice;
}
