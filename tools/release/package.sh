#!/usr/bin/env bash
# Packages a built Client for a GitHub release: a .dmg on macOS, a .zip on
# Windows. The `release` path of .github/workflows/ci.yml runs this on a `v*`
# tag; it is a script rather than inline workflow steps so that the exact
# packaging CI performs can be dry-run on a developer's machine.
#
#   client/tools/release/package.sh <build dir> <version> <output dir>
#
#   <build dir>   the configured product tree, e.g. client/build/ninja; the
#                 Release outputs are read from <build dir>/bin/Release
#   <version>     the version being released, e.g. 2.6.0 (no leading `v`)
#   <output dir>  receives the package plus <package>.notes.md, a fragment
#                 the release job pastes into the release notes
#
# Environment (all optional; each group is all-or-nothing):
#
#   MACOS_SIGNING_IDENTITY       a "Developer ID Application: ..." identity
#                                already in a keychain codesign can reach.
#                                Unset: the app is ad-hoc signed, and the
#                                notes say Gatekeeper will refuse it.
#   APPLE_ID, APPLE_TEAM_ID,     notarisation credentials for notarytool
#   APPLE_APP_SPECIFIC_PASSWORD  (requires MACOS_SIGNING_IDENTITY).
#   SCADA_SOURCE_DIRS            space-separated git checkouts whose remote and
#                                commit are recorded in SOURCES.txt -- the
#                                client and every sibling product it was built
#                                from, which is the Corresponding Source.
#
# THE DISPLAY RUNTIME IS NEVER PACKAGED HERE. The runtime binary's licence is
# not settled (superproject tasks.md 788), and a Client without it is a
# supported state: schematic windows report that there is no display runtime
# (ADR 0013). A build configured with SCADA_DISPLAY_RUNTIME_LIBRARY stages the
# library beside the executable, so this script removes it explicitly and then
# refuses a package that still contains one. Lifting this is a deliberate
# change to this script once 788 is closed, never a configure option.

set -euo pipefail

if [ $# -ne 3 ]; then
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 2
fi

build_dir=$(cd "$1" && pwd)
version=$2
mkdir -p "$3"
out_dir=$(cd "$3" && pwd)
bin_dir="$build_dir/bin/Release"
here=$(cd "$(dirname "$0")" && pwd)
client_root=$(cd "$here/../.." && pwd)
name_base="Telecontrol-SCADA-Client-$version"

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

die() { echo "package.sh: $*" >&2; exit 1; }

# One triplet directory under vcpkg_installed (plus vcpkg's own bookkeeping).
triplet_dir() {
  local d
  for d in "$build_dir"/vcpkg_installed/*/; do
    case "$(basename "$d")" in vcpkg) continue ;; esac
    [ -d "$d/share" ] && { echo "${d%/}"; return; }
  done
  die "no vcpkg_installed/<triplet> under $build_dir"
}

# The documents that travel with every package: the client's own licence and
# notices, the third-party licence of every vcpkg port linked into it, and
# where the exact sources are.
stage_documents() {
  local dest=$1 port_dir port
  cp "$client_root/LICENSE" "$client_root/COPYRIGHT" "$client_root/NOTICE" "$dest/"
  # LICENSE-EXCEPTION.txt is deliberately absent: it is a draft marked
  # NOT IN FORCE (tasks.md 788) and ships only once it grants something.

  # Every port's `copyright` file, which is where vcpkg puts the licence text
  # of what it built. Static linking (macOS) and DLLs (Windows) both
  # distribute these libraries, so their notices go with the binary.
  mkdir -p "$dest/THIRD-PARTY-LICENSES"
  for port_dir in "$(triplet_dir)"/share/*/; do
    port=$(basename "$port_dir")
    [ -f "$port_dir/copyright" ] &&
      cp "$port_dir/copyright" "$dest/THIRD-PARTY-LICENSES/$port.txt"
  done

  {
    echo "Telecontrol SCADA Client $version"
    echo
    echo "Built from these sources (GPL-3.0 Corresponding Source):"
    local d
    for d in ${SCADA_SOURCE_DIRS:-$client_root}; do
      if git -C "$d" rev-parse HEAD >/dev/null 2>&1; then
        printf '  %-16s %s @ %s\n' "$(basename "$d")" \
          "$(git -C "$d" remote get-url origin 2>/dev/null || echo '?')" \
          "$(git -C "$d" rev-parse HEAD)"
      fi
    done
  } > "$dest/SOURCES.txt"
}

refuse_display_runtime() {
  local found
  found=$(find "$1" -iname '*display_runtime*' -print)
  [ -z "$found" ] || die "the display runtime must not be packaged (tasks.md 788): $found"
}

# Starts the packaged executable offscreen and expects it still running after
# a few seconds. A missing DLL or dylib, or a plugin that cannot load, ends
# the process at once, so this is what proves the package is self-contained
# rather than working only beside the build tree. An optional second argument
# is the PATH the client alone runs with; the script's own tools keep theirs.
smoke_test() {
  local exe=$1 child_path=${2:-$PATH}
  echo "package.sh: smoke-testing $exe"
  PATH=$child_path QT_QPA_PLATFORM=offscreen "$exe" > "$work/smoke.log" 2>&1 &
  local pid=$!
  sleep 10
  if kill -0 "$pid" 2>/dev/null; then
    kill "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    echo "package.sh: still running after 10 s -- ok"
  else
    local rc=0
    wait "$pid" || rc=$?
    cat "$work/smoke.log" >&2
    die "the packaged client exited within 10 s (exit code $rc)"
  fi
}

package_macos() {
  local arch app_name app dmg notes identity minos entitlements
  arch=$(uname -m)
  app_name="Telecontrol SCADA Client.app"
  dmg="$out_dir/$name_base-macos-$arch.dmg"
  notes="$dmg.notes.md"
  identity=${MACOS_SIGNING_IDENTITY:-}
  entitlements="$client_root/app/qt/client.entitlements"

  [ -d "$bin_dir/client.app" ] || die "no $bin_dir/client.app; build Release first"

  local stage="$work/dmg"
  mkdir -p "$stage"
  app="$stage/$app_name"
  # ditto rather than cp -R: it preserves the bundle's extended attributes
  # and symlinks exactly, which codesign later checks.
  ditto "$bin_dir/client.app" "$app"
  rm -f "$app"/Contents/MacOS/*display_runtime*
  refuse_display_runtime "$app"

  # codesign refuses a bundle with data files under Contents/MacOS ("code
  # object is not signed at all / In subcomponent: .../client_ru.qm"), which
  # is where the build mirrors the catalogs because InstalledTranslation reads
  # applicationDirPath() + "/translations". Move them to Resources, where a
  # bundle's data belongs, and leave a symlink at the path the app reads:
  # a symlink is sealed as a symlink, and the lookup resolves through it.
  if [ -d "$app/Contents/MacOS/translations" ]; then
    mv "$app/Contents/MacOS/translations" "$app/Contents/Resources/translations"
    ln -s ../Resources/translations "$app/Contents/MacOS/translations"
  fi

  # Qt is static on arm64-osx, so the bundle should link nothing outside the
  # OS. A dylib from a build machine's prefix would load here and nowhere
  # else; refuse it rather than discover it on a user's Mac.
  local foreign
  foreign=$(otool -L "$app/Contents/MacOS/client" | tail -n +2 |
    awk '{print $1}' | grep -v -E '^(/usr/lib/|/System/Library/)' || true)
  [ -z "$foreign" ] || die "client links libraries outside the OS: $foreign"

  minos=$(otool -l "$app/Contents/MacOS/client" |
    awk '/LC_BUILD_VERSION/{f=1} f&&$1=="minos"{print $2; exit}')

  local docs="$stage/Documents"
  mkdir -p "$docs"
  stage_documents "$docs"
  ln -s /Applications "$stage/Applications"

  if [ -n "$identity" ]; then
    # --options runtime is the hardened runtime, which notarisation requires.
    # --timestamp is a secure timestamp from Apple, which it also requires.
    codesign --force --options runtime --timestamp \
      --entitlements "$entitlements" --sign "$identity" "$app"
  else
    # Ad hoc. The linker already ad-hoc signed the executable, but that seal
    # covers only the Mach-O; re-signing the bundle seals its resources and
    # translations too, so a damaged download is detected rather than run.
    codesign --force --entitlements "$entitlements" --sign - "$app"
  fi
  codesign --verify --strict --verbose=2 "$app"
  smoke_test "$app/Contents/MacOS/client"

  # hdiutil on hosted macOS runners intermittently fails with "Resource
  # busy"; a short retry is the usual remedy.
  rm -f "$dmg"
  local attempt
  for attempt in 1 2 3; do
    if hdiutil create -volname "Telecontrol SCADA Client $version" \
        -srcfolder "$stage" -ov -format UDZO "$dmg"; then
      break
    fi
    [ "$attempt" = 3 ] && die "hdiutil create failed three times"
    sleep 5
  done

  local signing
  if [ -n "$identity" ]; then
    codesign --force --timestamp --sign "$identity" "$dmg"
    if [ -n "${APPLE_ID:-}" ]; then
      # Notarising the disk image covers the app inside it; stapling attaches
      # the ticket so Gatekeeper can check it offline.
      xcrun notarytool submit "$dmg" --wait \
        --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
        --password "$APPLE_APP_SPECIFIC_PASSWORD"
      xcrun stapler staple "$dmg"
      xcrun stapler validate "$dmg"
      spctl --assess --type open --context context:primary-signature -v "$dmg"
      signing="Signed with a Developer ID and notarised by Apple."
    else
      signing="Signed with a Developer ID but **not notarised**: macOS will refuse the first launch. Allow it in *System Settings → Privacy & Security → Open Anyway*."
    fi
  else
    # Control-click → Open no longer bypasses Gatekeeper since macOS 15, so
    # the notes give the two routes that still work.
    signing="**Not signed or notarised.** macOS will refuse to open it from a download. After the first refusal, allow it in *System Settings → Privacy & Security → Open Anyway*, or run \`xattr -dr com.apple.quarantine \"/Applications/$app_name\"\` once after copying it."
  fi

  cat > "$notes" <<EOF
**macOS** — \`$(basename "$dmg")\`: Apple silicon ($arch), macOS ${minos:-?} or later. Open the disk image and drag the app to *Applications*. $signing
EOF
  echo "package.sh: wrote $dmg"
}

package_windows() {
  local zip_name stage notes plugins crt
  zip_name="$name_base-windows-x64"
  stage="$work/$zip_name"
  notes="$out_dir/$zip_name.zip.notes.md"
  mkdir -p "$stage"

  [ -f "$bin_dir/client.exe" ] || die "no $bin_dir/client.exe; build Release first"
  cp "$bin_dir/client.exe" "$stage/"
  cp -R "$bin_dir/translations" "$stage/translations"

  # Qt is shared on x64-windows. Its plugins are not deployed beside the build
  # outputs (the CI test step points QT_PLUGIN_PATH at vcpkg's copy instead),
  # so the package carries vcpkg's whole plugin tree: `plugins/` beside the
  # executable is on Qt's default search path. All of it rather than a picked
  # list, because nothing here can launch the Windows client to find out
  # which one a hand-picked list forgot.
  plugins="$(triplet_dir)/Qt6/plugins"
  [ -d "$plugins" ] || die "no Qt plugins at $plugins"
  cp -R "$plugins" "$stage/plugins"

  # The DLL closure. vcpkg copies every DLL any target needs into the shared
  # bin/Release, test binaries' included, so the package takes exactly what
  # client.exe and the plugins import, transitively, and resolves each from
  # there. A name not found there is a system DLL, or the CRT handled below.
  local queue=("$stage/client.exe") f dll
  while IFS= read -r -d '' f; do queue+=("$f"); done \
    < <(find "$stage/plugins" -iname '*.dll' -print0)
  declare -A seen=()
  while [ ${#queue[@]} -gt 0 ]; do
    f=${queue[0]}
    queue=("${queue[@]:1}")
    for dll in $(dumpbin //nologo //dependents "$f" |
        grep -io '^ *[a-z0-9_.+-]*\.dll' | tr -d ' '); do
      local key=${dll,,}
      [ -n "${seen[$key]:-}" ] && continue
      seen[$key]=1
      if [ -f "$bin_dir/$dll" ]; then
        cp "$bin_dir/$dll" "$stage/"
        queue+=("$stage/$dll")
      fi
    done
  done

  # The MSVC runtime, so the client starts on a machine without the Visual
  # C++ Redistributable. VCToolsRedistDir is set by the developer command
  # prompt (ilammy/msvc-dev-cmd in CI); the files under its CRT directory are
  # the ones Microsoft designates for app-local redistribution.
  [ -n "${VCToolsRedistDir:-}" ] || die "VCToolsRedistDir is not set; run from a developer command prompt"
  local crts=("$(cygpath -u "$VCToolsRedistDir")"/x64/Microsoft.VC*.CRT)
  crt=${crts[0]}
  [ -d "$crt" ] || die "no MSVC CRT redist under $VCToolsRedistDir"
  cp "$crt"/*.dll "$stage/"

  refuse_display_runtime "$stage"
  stage_documents "$stage"

  # Run with nothing but the OS on PATH, so a DLL the closure missed fails
  # here instead of being found in the build tree.
  smoke_test "$stage/client.exe" "/c/Windows/System32:/c/Windows"

  rm -f "$out_dir/$zip_name.zip"
  (cd "$work" && 7z a -tzip -bd "$out_dir/$zip_name.zip" "$zip_name" > /dev/null)

  cat > "$notes" <<EOF
**Windows** — \`$zip_name.zip\`: Windows 10 or later, x64. Unzip anywhere and run \`client.exe\`. **Not code-signed**: Windows SmartScreen may warn on first launch; choose *More info* → *Run anyway*.
EOF
  echo "package.sh: wrote $out_dir/$zip_name.zip"
}

case "$(uname -s)" in
  Darwin) package_macos ;;
  MINGW*|MSYS*|CYGWIN*) package_windows ;;
  *) die "packaging is defined for macOS and Windows only" ;;
esac
