#!/usr/bin/env bash
# Run in MSYS2 MINGW64 after building. ldd reports transitive dependencies.
set -euo pipefail
stage=dist/tigervnc-mac-server-windows-x64
mkdir -p "$stage"
cp build/vncviewer/vncviewer.exe "$stage/"
ldd build/vncviewer/vncviewer.exe > dist/dependencies.txt
if grep -q 'not found' dist/dependencies.txt; then
  cat dist/dependencies.txt
  exit 1
fi
while IFS= read -r dll; do
  cp "$dll" "$stage/"
done < <(awk '/=> \/mingw64\// {print $3}' dist/dependencies.txt)
cp -r /mingw64/share/licenses "$stage/third-party-licenses"
cp LICENCE.TXT "$stage/"
cp doc/macos-compatibility.md "$stage/"
cp packaging/windows-release-notes.md "$stage/README.md"
printf '@echo off\r\nstart "" "%%~dp0vncviewer.exe" -MacServer=1 -MacOSOptionKey=1 -RemoteResize=0 %%*\r\n' > "$stage/Mac server.cmd"
(cd dist && zip -r tigervnc-mac-server-windows-x64.zip tigervnc-mac-server-windows-x64)
(cd dist && sha256sum *.zip > SHA256SUMS)
# Keep the upload limited to the archive, checksum and dependency manifest.
rm -r "$stage"
