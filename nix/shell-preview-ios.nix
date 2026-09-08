# shell-preview for iOS: the pure half as static archives (pkgs.mkIosCmakeStage),
# plus run-ios-sim, the impure Xcode-generator link and simctl step.
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

  runIosSim = buildPkgs.writeShellApplication {
    name = "run-ios-sim";
    runtimeInputs = [ buildPkgs.cmake pkgs.xcodeWrapper ];
    text = ''
      # The stages were gated at build time; the link step runs later, so
      # gate again before touching xcodebuild.
      ${pkgs.xcodeWrapper.versionGate}

      app_src=${src}/shell-preview/platform/ios/app
      bundle_id=co.logos.basecamp.shellpreview
      # keyed on the stage store path: CMake refuses a cache made from other inputs
      build_dir="''${LOGOS_IOS_SHELL_PREVIEW_BUILD_DIR:-''${TMPDIR:-/tmp}/basecamp-shell-preview-ios/$(basename ${stage})}"
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
        "-DBASECAMP_FIXTURE=${src}/shell-preview/fixtures/shell-fixture.json"

      echo "==> xcodebuild (unsigned, ${appleSdk}; full log: $build_dir/xcodebuild.log)"
      app="$build_dir/Debug-${appleSdk}/BasecampShellPreview.app"
      rm -rf "$app"
      # errexit+pipefail would exit on grep's status before the message below
      set +e
      xcodebuild -project "$build_dir/BasecampShellPreviewIos.xcodeproj" -target BasecampShellPreview \
        -configuration Debug -sdk ${appleSdk} -arch arm64 \
        CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY= \
        build 2>&1 | tee "$build_dir/xcodebuild.log" | grep -E '^\*\*|error:|warning: .*ld'
      xcode_status=''${PIPESTATUS[0]}
      set -e
      if [ "$xcode_status" -ne 0 ]; then
        echo "xcodebuild failed; see $build_dir/xcodebuild.log" >&2
        exit 1
      fi
      [ -d "$app" ] || { echo "xcodebuild produced no $app" >&2; exit 1; }

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
      # simctl returns 0 whatever the app did, so an attached app that is gone
      # within seconds is treated as the startup qFatal it almost always is.
      launched=$(date +%s)
      xcrun simctl launch --console-pty "$udid" "$bundle_id" "$@"
      if [ $(( $(date +%s) - launched )) -lt 5 ]; then
        echo "app exited right after launch; the console output above says why" >&2
        exit 1
      fi
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
