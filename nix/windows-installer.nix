# NSIS installer for a bundled Windows Basecamp tree.
#
# `pkgs` is the BUILD system's, never the target's. makensis, objdump and the
# icon conversion RUN on the builder, so reaching for packages.x86_64-windows
# here would put a PE where a native binary has to execute -- the same rule lgx
# and nix-bundle-dir already follow.
#
# Deliberately kept in this repo rather than started as a sibling of
# nix-bundle-appimage / nix-bundle-macos-app: Basecamp is the only consumer
# today. It is written as a plain function over `bundle` so that lifting it into
# a nix-bundle-nsis flake later is a move rather than a rewrite.
{ pkgs
, bundle
, version
, name ? "Logos Basecamp"
, exeName ? "LogosBasecamp.exe"
, publisher ? "Logos"
, icon
}:

let
  # A Windows file name has no room for the space in `name`, and this is the
  # string a user downloads and later searches for.
  slug = "logos-basecamp";
in
pkgs.runCommand "${slug}-installer-${version}"
  {
    nativeBuildInputs = [ pkgs.nsis pkgs.imagemagick pkgs.binutils ];
    meta.description = "Windows installer for ${name} ${version}";
  }
  ''
    set -eo pipefail
    mkdir -p "$out"

    # -L DEREFERENCES, and that is the whole ballgame. A third of the bundle's
    # bin/ reaches its DLLs through symlinks into /nix/store; an installer built
    # over those links ships a tree that dies with 0xC0000135 before main(),
    # with no output at all. It is the same failure nix-bundle-lgx once shipped
    # by letting `cp -a` imply --no-dereference.
    stage="$PWD/stage"
    mkdir -p "$stage"
    cp -rL --no-preserve=mode,ownership ${bundle}/. "$stage/"

    staged=$(find "$stage" -type f | wc -l | tr -d ' ')
    echo "staging $staged file(s) from ${bundle}"
    [ "$staged" -gt 0 ] || { echo "the bundle staged empty"; exit 1; }

    # Assert the dereference DIRECTLY rather than inferring it from a file
    # count. A staging that quietly kept links is invisible in an exit code,
    # and this is the only check standing between that and a shipped installer.
    links=$(find "$stage" -type l | wc -l | tr -d ' ')
    [ "$links" -eq 0 ] || {
      echo "$links symlink(s) survived staging; the installed tree would carry"
      echo "dangling links into /nix/store and die 0xC0000135 before main()"
      find "$stage" -type l | head -5
      exit 1
    }
    [ -f "$stage/bin/${exeName}" ] || {
      echo "no bin/${exeName} staged; the installer would have nothing to launch"
      exit 1
    }

    # The installer's whole user-visible promise is a GUI that opens with no
    # console behind it, and a CUI image breaks that silently. Assert it where
    # the artifact is made rather than trusting that a CMake property survived.
    # Fails closed: a reader that cannot parse the PE yields an empty string,
    # which is not 00000002 either.
    subsystem=$(objdump -p "$stage/bin/${exeName}" 2>/dev/null \
                  | sed -n 's/^Subsystem[[:space:]]*\([0-9]*\).*/\1/p' | head -1)
    if [ "$subsystem" != "00000002" ]; then
      echo "${exeName} reports PE subsystem '$subsystem', expected 00000002 (Windows GUI)."
      echo "A console-subsystem image is handed a console window by the OS before"
      echo "main() runs. Set WIN32_EXECUTABLE on the target."
      exit 1
    fi

    magick ${icon} -define icon:auto-resize=256,128,64,48,32,16 "$PWD/app.ico"

    # VIProductVersion takes exactly four numeric fields, so a tag like
    # 0.0.0-dev has to be reduced rather than passed through.
    quad=$(printf '%s' "${version}" | sed 's/[^0-9.].*$//' \
             | awk -F. '{printf "%d.%d.%d.%d", $1+0, $2+0, $3+0, $4+0}')

    out_exe="$out/${slug}-${version}-x86_64-setup.exe"
    makensis -V3 \
      -DAPPNAME="${name}" \
      -DAPPVERSION="${version}" \
      -DAPPVERSION_QUAD="$quad" \
      -DAPPPUBLISHER="${publisher}" \
      -DAPPEXE="${exeName}" \
      -DAPPICON="$PWD/app.ico" \
      -DSTAGEDIR="$stage" \
      -DOUTFILE="$out_exe" \
      ${./nsis/logos-basecamp.nsi}

    [ -f "$out_exe" ] || { echo "makensis produced no installer"; exit 1; }
    size=$(stat -c %s "$out_exe")
    echo "installer: $(( size / 1024 / 1024 )) MB from $staged staged file(s)"
    # Solid LZMA over this tree lands near 65 MB. Under 16 means File /r matched
    # a fraction of it, which makensis reports as success -- a wildcard that
    # matches SOMETHING is not an error.
    [ "$size" -gt $((16 * 1024 * 1024)) ] || {
      echo "installer is only $(( size / 1024 / 1024 )) MB; the payload is short"
      exit 1
    }
  ''
