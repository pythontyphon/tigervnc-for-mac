# macOS compatibility and deployment

Target: Ubuntu 26.04 **x86-64 (amd64)** clients connecting to Apple's built-in
Screen Sharing / Remote Management server. Research date: 2026-09-17.
[Apple lists macOS 27 as current](https://support.apple.com/en-by/100100).
End-to-end behavior on that release still needs a live Mac acceptance test.

## Changes

* **Options → Input → Mouse → Mac server** (`MacServer=1`) enables paced
  scrolling and is saved in connection profiles. Multiplier 1 automatically
  selects 12 steps in this mode; 2–50 overrides the amount. Events are sent
  one step per axis every 8 ms, not in a single burst. Pending work is bounded
  to 24 steps per axis (about 192 ms), discarding excess input under load.
  Reversing direction replaces queued steps on that axis. Moving the pointer,
  clicking, leaving the viewer, losing focus, or changing options cancels the
  tail so it cannot scroll a different control. No synthetic momentum is added.
  This pacing is experimental and needs a live Mac comparison.

* `ScrollWheelSpeed=1..50` repeats standard RFB wheel press/release pairs;
  start with **12** for Apple's slow scrolling. Default is 1 for other servers.
* Linux/Windows wheel magnitude is preserved. Both axes get independent events;
  held mouse buttons stay held. Work is capped at 1000 steps per axis/event,
  with overflow-safe arithmetic. FLTK's macOS client backend uses different
  units, so its existing unit-step behavior is retained before multiplication.
* Optional `MacOSOptionKey=1` sends left Alt as `Mode_switch`, right Alt as
  `ISO_Level3_Shift`. Super/Windows remains Command; Control is unchanged.
  Existing connection key tracking uses the same mapped symbol on release.
  This follows TigerVNC's existing macOS-client Option mapping; confirm it
  with your server and keyboard layout.
* These settings are in **Options → Input** and saved in connection profiles.

## Install

Download and unzip `tigervnc-macos-ubuntu26.04-amd64` from the
[Ubuntu macOS viewer workflow](https://github.com/pythontyphon/tigervnc-for-mac/actions/workflows/macos-viewer.yml):

```sh
sha256sum -c SHA256SUMS
sudo apt install ./tigervnc-macos-viewer_*_amd64.deb
mac-vnc mac-hostname
```

`mac-vnc` selects Mac server mode, multiplier 12, Option mapping, and `RemoteResize=0`.
Override settings after the command:

```sh
mac-vnc -ScrollWheelSpeed=20 mac-hostname
mac-vnc -MacOSOptionKey=0 mac-hostname
vncviewer-macos other-vnc-host    # upstream defaults
```

The package coexists with the distribution viewer. Remove it with
`sudo apt remove tigervnc-macos-viewer`. It targets Ubuntu 26.04 amd64, not
every Linux distribution; rebuild on other distributions. The viewer uses
X11/XWayland, like upstream.

Standard VNC does not encrypt screen/input traffic. If Remote Login is enabled
on the Mac, an SSH tunnel provides encryption (leave the first command running):

```sh
ssh -N -L 127.0.0.1:5901:localhost:5900 user@mac-hostname
mac-vnc 127.0.0.1::5901
```

GnuTLS and Nettle remain enabled, including upstream DH authentication for
Apple account login. If authentication fails, inspect the error first. Apple's
separate “VNC viewers may control screen with password” setting enables standard
VNC authentication with a separate password. Do not bypass key-length checks.

## Research findings

| Area | Evidence and response |
| --- | --- |
| Scrolling | [TigerVNC #1741](https://github.com/TigerVNC/tigervnc/issues/1741) reports the issue on Linux, Windows and other viewers (RealVNC, UltraVNC, KRDC). The maintainer attributes it to Apple's interpretation of standard wheel steps. Our multiplier compensates for that behavior; it does not implement Apple's private scroll events. |
| Lost magnitude | Upstream `Viewport::handle(FL_MOUSEWHEEL)` only checks the sign of wheel deltas. [gitcnd's patch](https://github.com/gitcnd/tigervnc/commit/381d8aee) suggested magnitude preservation and a multiplier. This fork independently implements bounded event generation and tests. Linux normally supplies one step per event, so the multiplier is the main improvement there. |
| Option key | [#770](https://github.com/TigerVNC/tigervnc/issues/770) documents Linux Alt being interpreted as Command. [RealVNC's reference](https://www.realvnc.com/en/developer/docs/latest/api/c/Keyboard.h.html) identifies Level3 Shift as Mac Option. Our mapping is explicit and reversible. The local desktop can still intercept Alt+Tab/Super; use the viewer's keyboard grab when needed. |
| Clipboard | [User reports](https://groups.google.com/g/tigervnc-users/c/0-PVwHWbLkQ) describe asymmetric transfer; [independent traced sessions](https://github.com/riazhassan-za/openvncviewer) report Apple ignoring standard cut-text messages in favor of private machinery. No native Apple clipboard fix is claimed. For occasional text transfer, SSH `pbcopy`/`pbpaste` can be paired with `wl-copy`/`wl-paste` or `xclip`. |
| Cursor | [#826](https://github.com/TigerVNC/tigervnc/issues/826) reports stale shapes. `AlwaysCursor=1 -CursorType=System` helps only when the server supplies an invisible cursor; it cannot repair a stale visible shape. No speculative cursor patch is applied. |
| Retina / resizing | `RemoteResize` asks the server to change resolution; it is not client scaling. Apple support for the standard request is not assured. The launcher disables it. Choose a suitable resolution on the Mac; this fork adds no scaling engine. |
| Login / protocol errors | Historical [#51](https://github.com/TigerVNC/tigervnc/issues/51) and [#137](https://github.com/TigerVNC/tigervnc/issues/137) concern authentication or unknown messages. Current upstream already has DH authentication. Historical reports do not establish a current macOS 27 failure. |

The [gitcnd release](https://github.com/gitcnd/tigervnc/releases/tag/v1.6.80)
advertises clipboard, keyboard and scaling fixes, but its source diff against
its upstream base contains only wheel-related implementation changes. Those
other claims are not treated as verified fixes.

## Build on Ubuntu 26.04

```sh
sudo apt install build-essential cmake pkg-config dpkg-dev \
  libfltk1.3-dev libpixman-1-dev libjpeg-turbo8-dev zlib1g-dev \
  libgnutls28-dev nettle-dev libgmp-dev libgtest-dev \
  libxtst-dev libxdamage-dev libxfixes-dev libxrandr-dev \
  libxi-dev libxinerama-dev libxcursor-dev
cmake -S . -B build -DBUILD_VIEWER=ON -DENABLE_GNUTLS=ON \
  -DENABLE_NETTLE=ON -DENABLE_WAYLAND=OFF -DENABLE_NLS=OFF \
  -DENABLE_H264=OFF -DENABLE_AUDIO=OFF -DENABLE_PAM=OFF \
  -DENABLE_SYSTEMD=OFF -DENABLE_PWQUALITY=OFF
cmake --build build --parallel
ctest --test-dir build/tests/unit --output-on-failure
bash packaging/build-deb.sh
```

## Windows and iOS/iPadOS

The Windows Mac server viewer workflow builds and unit-tests a Release-mode
x64 viewer, bundles its runtime DLLs, and creates a portable ZIP with SHA256SUMS.
Tags matching `windows-*` publish a GitHub prerelease; manual workflow runs
produce an Actions artifact. Extract the complete ZIP and run `vncviewer.exe`,
then check **Options → Input → Mouse → Mac server**. `Mac server.cmd` selects
that mode plus Option mapping and disables remote resizing.
 There is no iOS/iPadOS target in TigerVNC's native build system;
those devices need another viewer or a separate mobile project. A browser
viewer plus gateway could share a UI but still requires Apple-specific input
work and another service; it is not a demonstrated universal fix.

## Mac acceptance test

1. Record `sw_vers`, keyboard layout and Linux X11/Wayland session type.
2. Compare Mac server mode off/on and multiplier 6 and 12 in Safari/Chrome, Finder and Terminal. Tune down
   if Terminal overscrolls. Try fast wheels, trackpads and both axes.
3. Scroll while dragging, then release; check that no mouse button stays held.
4. Test Option+arrow, Command+C/V, Control, right Alt/AltGr, and key release
   after losing focus. Disable Option mapping if it does not suit your layout.
5. Test view-only, reconnect, and saved profiles; check a non-Apple server
   with default settings for ordinary scrolling.

Automated tests cover event counts/order, directions, held buttons, zero and
extreme deltas, and modifier mapping. A virtual-display integration test also
drives the real viewer and checks its RFB messages for scroll multiplication,
held buttons, mapped key release, unchanged defaults, and view-only mode:

```sh
sudo apt install xvfb xauth xdotool
xvfb-run -a python3 tests/remote-input-wire.py build/vncviewer/vncviewer
```

These checks cannot establish behavior on a Mac that was not available during
development.

## Why native Screen Sharing can feel different

[RFB 7.5.5](https://www.rfc-editor.org/rfc/rfc6143.html#section-7.5.5)
encodes wheel movement as button press/release pairs, without pixel deltas,
gesture phase or momentum. Pacing these pairs addresses burstiness but cannot
supply those missing semantics. FLTK 1.3 also limits the input available here.

[Apple documents](https://support.apple.com/en-au/guide/mac-help/mchl1883115d/mac)
a separate High Performance mode with 30/60 FPS and low latency on supported
Apple silicon Macs. That is another possible contributor to perceived
smoothness, not proof of which input path was used in a particular session.
Native pixel scrolling would require verifying Apple's negotiated input
extensions (including units, phases and compatibility) with a live Mac and
implementing them alongside a standard-RFB fallback. This release does not
send speculative private messages or claim native Screen Sharing parity.
