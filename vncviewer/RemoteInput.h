/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __REMOTEINPUT_H__
#define __REMOTEINPUT_H__

#include <algorithm>
#include <cstdint>
// FLTK may already have included the system X11 keysyms. Including our
// bundled copy again can redefine symbols on older X11 installations.
#ifndef XK_ISO_Level3_Shift
#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_LATIN1
#include <rfb/keysymdef.h>
#endif

namespace remoteInput {

// Limit work per GUI event. Widen before negation/multiplication so even
// INT_MIN from an input backend cannot overflow or stall the viewer.
inline int wheelSteps(int delta, int multiplier)
{
  int64_t magnitude = delta;
  if (magnitude < 0)
    magnitude = -magnitude;
  return static_cast<int>((std::min<int64_t>)(
    1000, magnitude * (std::max)(1, (std::min)(50, multiplier))));
}

template<typename Send>
void wheel(int dx, int dy, int multiplier, uint16_t heldButtons, Send send)
{
  const int deltas[] = {dy, dx};
  for (int axis = 0; axis < 2; ++axis) {
    const uint16_t mask = 1 << (3 + axis * 2 + (deltas[axis] > 0));
    for (int step = 0; step < wheelSteps(deltas[axis], multiplier); ++step) {
      send(heldButtons | mask);
      send(heldButtons);
    }
  }
}

// Pace at most one step per axis per timer tick. Bound the tail to 192 ms
// (24 ticks at 8 ms), discarding excess input instead of building seconds of
// delayed scrolling. Reversals replace the old direction immediately.
class PacedWheel {
public:
  void clear() { pending[0] = pending[1] = 0; }
  bool empty() const { return pending[0] == 0 && pending[1] == 0; }
  void add(int dx, int dy, int multiplier) {
    const int deltas[] = {dx, dy};
    for (int axis = 0; axis < 2; ++axis) {
      if (!deltas[axis]) continue;
      const int direction = deltas[axis] > 0 ? 1 : -1;
      if ((pending[axis] > 0) != (direction > 0)) pending[axis] = 0;
      const int steps = wheelSteps(deltas[axis], multiplier);
      pending[axis] = (std::max)(-24, (std::min)(24,
        pending[axis] + direction * steps));
    }
  }
  template<typename Send>
  void tick(uint16_t heldButtons, Send send) {
    const int dx = (pending[0] > 0) - (pending[0] < 0);
    const int dy = (pending[1] > 0) - (pending[1] < 0);
    pending[0] -= dx;
    pending[1] -= dy;
    wheel(dx, dy, 1, heldButtons, send);
  }
private:
  int pending[2] = {0, 0};
};

// Apple treats Alt as Command. Use the Option/AltGr keysyms, matching
// TigerVNC's existing macOS-client mapping. Super remains Command.
inline uint32_t macOSKey(uint32_t keySym)
{
  switch (keySym) {
  case XK_Alt_L: return XK_Mode_switch;
  case XK_Alt_R: return XK_ISO_Level3_Shift;
  default: return keySym;
  }
}

}
#endif
