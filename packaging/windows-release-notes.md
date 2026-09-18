Windows x64 portable TigerVNC viewer with a **Mac server** checkbox under
**Options → Input → Mouse**. Extract the whole ZIP, keeping its DLLs beside
`vncviewer.exe`, then run the viewer. `Mac server.cmd` also enables Option-key
mapping and disables remote resizing.

Mac server mode sends wheel steps at 8 ms intervals instead of one burst.
Multiplier 1 selects 12 steps per notch in this mode; 2–50 selects a custom
amount. Pending work is capped at 24 steps per axis, so fast input can saturate.
Direction changes discard the old direction; mouse movement, clicks, focus
loss and options changes cancel queued scrolling.

This is an experimental standard-VNC workaround, not Apple's native pixel
scrolling or High Performance protocol. Scrolling feel needs testing against
a real Mac. Disable Mac server to restore ordinary immediate wheel events.
The Windows archive is built and unit-tested by GitHub Actions; it is unsigned.
