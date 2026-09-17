#!/usr/bin/env python3
"""Exercise the real viewer against a loopback RFB server under Xvfb.

Usage: xvfb-run -a python3 tests/remote-input-wire.py build/vncviewer/vncviewer
Requires xdotool. No real desktop or remote Mac is contacted.
"""
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time


def receive(conn, length):
    data = b""
    while len(data) < length:
        chunk = conn.recv(length - len(data))
        if not chunk:
            raise EOFError()
        data += chunk
    return data


def wait_for(predicate, message):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.05)
    raise AssertionError(message)


def run_case(viewer, multiplier, mapping, view_only=False):
    pointers, keys, errors = [], [], []
    ready = threading.Event()
    stopping = threading.Event()
    with socket.socket() as listener, tempfile.TemporaryFile(mode="w+") as log:
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        listener.settimeout(10)
        port = listener.getsockname()[1]

        def server():
            try:
                with listener.accept()[0] as conn:
                    conn.settimeout(15)
                    conn.sendall(b"RFB 003.008\n")
                    assert receive(conn, 12) == b"RFB 003.008\n"
                    conn.sendall(b"\x01\x01")  # None, loopback test only
                    assert receive(conn, 1) == b"\x01"
                    conn.sendall(b"\0\0\0\0")
                    receive(conn, 1)  # ClientInit
                    name = b"RemoteInputWireTest"
                    pf = struct.pack(">BBBBHHHBBBxxx", 32, 24, 0, 1, 255, 255, 255, 16, 8, 0)
                    conn.sendall(struct.pack(">HH", 320, 240) + pf +
                                 struct.pack(">I", len(name)) + name)
                    while not stopping.is_set():
                        kind = receive(conn, 1)[0]
                        if kind == 0:
                            receive(conn, 19)  # SetPixelFormat
                        elif kind == 2:
                            count = struct.unpack(">xH", receive(conn, 3))[0]
                            receive(conn, 4 * count)
                        elif kind == 3:
                            receive(conn, 9)
                            # Return one empty update, avoiding a busy request loop.
                            if not ready.is_set():
                                conn.sendall(b"\0\0\0\0")
                                ready.set()
                        elif kind == 4:
                            down, symbol = struct.unpack(">BxxI", receive(conn, 7))
                            keys.append((down, symbol))
                        elif kind == 5:
                            mask, _, _ = struct.unpack(">BHH", receive(conn, 5))
                            pointers.append(mask)
                        elif kind == 6:
                            length = struct.unpack(">xxxI", receive(conn, 7))[0]
                            receive(conn, length)
                        else:
                            raise AssertionError(f"Unexpected client message {kind}")
            except (EOFError, OSError) as exc:
                if not stopping.is_set():
                    errors.append(exc)
            except Exception as exc:
                errors.append(exc)

        worker = threading.Thread(target=server, daemon=True)
        worker.start()
        proc = subprocess.Popen([
            viewer, "-SecurityTypes=None", "-AlertOnFatalError=0",
            "-ReconnectOnError=0", "-RemoteResize=0", "-SendClipboard=0",
            f"-ScrollWheelSpeed={multiplier}", f"-MacOSOptionKey={int(mapping)}",
            f"-ViewOnly={int(view_only)}", f"127.0.0.1::{port}",
        ], stdout=log, stderr=log)
        try:
            wait_for(ready.is_set, "viewer did not complete RFB handshake")
            window = subprocess.check_output([
                "xdotool", "search", "--sync", "--onlyvisible", "--name",
                "RemoteInputWireTest",
            ], text=True, timeout=10).splitlines()[0]
            subprocess.run(["xdotool", "windowfocus", "--sync", window,
                            "mousemove", "--window", window, "100", "100"],
                           check=True, timeout=10)
            time.sleep(0.2)
            start = len(pointers)
            subprocess.run(["xdotool", "mousedown", "1", "click", "4",
                            "click", "7", "mouseup", "1",
                            "keydown", "Alt_L", "keyup", "Alt_L"],
                           check=True, timeout=10)
            if view_only:
                time.sleep(0.4)
                assert pointers[start:] == [] and keys == [], (pointers, keys)
            else:
                expected_key = 0xff7e if mapping else 0xffe9
                wait_for(lambda: (0, expected_key) in keys,
                         "mapped key release missing")
                # Include held left button throughout each scroll pair.
                actual = pointers[start:]
                expected = [1] + [9, 1] * multiplier + [65, 1] * multiplier + [0]
                # Position-only pointer updates can precede the button press.
                while actual and actual[0] == 0:
                    actual = actual[1:]
                assert actual[:len(expected)] == expected, (actual, expected)
                assert keys == [(1, expected_key), (0, expected_key)], keys
            assert not errors, errors
        except Exception:
            log.seek(0)
            print(log.read(), file=sys.stderr)
            raise
        finally:
            stopping.set()
            proc.terminate()
            proc.wait(timeout=5)
            worker.join(timeout=5)


if __name__ == "__main__":
    run_case(sys.argv[1], 12, True)
    run_case(sys.argv[1], 1, False)
    run_case(sys.argv[1], 12, True, view_only=True)
    print("Wire tests passed: scrolling, held buttons, key release, defaults, view-only")
