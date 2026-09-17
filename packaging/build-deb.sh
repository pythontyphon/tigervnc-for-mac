#!/usr/bin/env bash
set -euo pipefail
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
binary=$(realpath "${1:-$repo/build/vncviewer/vncviewer}")
output=$(realpath -m "${2:-$repo/dist}")
version="1.16.80+macos.$(git -C "$repo" rev-parse --short=12 HEAD)"
arch=$(dpkg --print-architecture)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/root/DEBIAN" "$stage/root/usr/bin" \
  "$stage/root/usr/share/doc/tigervnc-macos-viewer" "$stage/debian" "$output"
install -m 755 "$binary" "$stage/root/usr/bin/vncviewer-macos"
install -m 755 "$repo/packaging/mac-vnc" "$stage/root/usr/bin/mac-vnc"
install -m 644 "$repo/LICENCE.TXT" "$stage/root/usr/share/doc/tigervnc-macos-viewer/copyright"
install -m 644 "$repo/doc/macos-compatibility.md" "$stage/root/usr/share/doc/tigervnc-macos-viewer/"
cat > "$stage/debian/control" <<'EOF'
Source: tigervnc-macos-viewer
Section: net
Priority: optional
Maintainer: pythontyphon <pythontyphon@users.noreply.github.com>

Package: tigervnc-macos-viewer
Architecture: any
Description: TigerVNC viewer with macOS input compatibility options
EOF
deps=$(cd "$stage" && dpkg-shlibdeps -O -e"$stage/root/usr/bin/vncviewer-macos")
deps=${deps#shlibs:Depends=}
cat > "$stage/root/DEBIAN/control" <<EOF
Package: tigervnc-macos-viewer
Version: $version
Architecture: $arch
Maintainer: pythontyphon <pythontyphon@users.noreply.github.com>
Section: net
Priority: optional
Depends: $deps
Homepage: https://github.com/pythontyphon/tigervnc-for-mac
Description: TigerVNC viewer with macOS input compatibility options
 Configurable wheel speed and Option key mapping for Apple's VNC server.
 Installs as mac-vnc and vncviewer-macos alongside the distribution viewer.
EOF
dpkg-deb --root-owner-group --build "$stage/root" \
  "$output/tigervnc-macos-viewer_${version}_${arch}.deb"
