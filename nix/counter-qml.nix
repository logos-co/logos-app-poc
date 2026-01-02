# Packages the QML-only counter plugin (no build step)
{ pkgs, common, src }:

pkgs.stdenv.mkDerivation {
  pname = "${common.pname}-counter-qml-plugin";
  version = common.version;

  inherit src;
  inherit (common) meta;

  dontUnpack = true;
  phases = [ "installPhase" ];

  installPhase = ''
    runHook preInstall

    dest="$out/qml_plugins/counter_qml"
    mkdir -p "$dest/icons"

    cp ${src}/logos_dapps/counter_qml/Main.qml "$dest/Main.qml"
    cp ${src}/logos_dapps/counter_qml/metadata.json "$dest/metadata.json"
    cp ${src}/logos_dapps/counter_qml/icons/counter.png "$dest/icons/counter.png"

    runHook postInstall
  '';
}
