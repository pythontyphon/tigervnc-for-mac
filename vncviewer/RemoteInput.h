/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __REMOTEINPUT_H__
#define __REMOTEINPUT_H__

#include <algorithm>
#include <cstdint>
#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_LATIN1
#include <rfb/keysymdef.h>

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
