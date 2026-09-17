/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <gtest/gtest.h>
#include <climits>
#include <vector>
#include "RemoteInput.h"

TEST(RemoteInput, WheelMagnitudeAndHeldButtons)
{
  std::vector<uint16_t> events;
  remoteInput::wheel(0, -3, 2, 5,
                     [&](uint16_t mask) { events.push_back(mask); });
  ASSERT_EQ(events.size(), 12u);
  for (size_t i = 0; i < events.size(); i += 2) {
    EXPECT_EQ(events[i], 13);
    EXPECT_EQ(events[i + 1], 5);
  }
}

TEST(RemoteInput, AxesHaveIndependentPressReleasePairs)
{
  std::vector<uint16_t> events;
  remoteInput::wheel(-1, 2, 1, 0,
                     [&](uint16_t mask) { events.push_back(mask); });
  EXPECT_EQ(events, (std::vector<uint16_t>{16, 0, 16, 0, 32, 0}));
  events.clear();
  remoteInput::wheel(1, -1, 1, 0,
                     [&](uint16_t mask) { events.push_back(mask); });
  EXPECT_EQ(events, (std::vector<uint16_t>{8, 0, 64, 0}));
}

TEST(RemoteInput, ZeroAndExtremeDeltas)
{
  int count = 0;
  remoteInput::wheel(0, 0, 12, 0, [&](uint16_t) { ++count; });
  EXPECT_EQ(count, 0);
  EXPECT_EQ(remoteInput::wheelSteps(INT_MIN, 50), 1000);
  EXPECT_EQ(remoteInput::wheelSteps(INT_MAX, INT_MAX), 1000);
  EXPECT_EQ(remoteInput::wheelSteps(1, 12), 12);
  EXPECT_EQ(remoteInput::wheelSteps(-1, 1), 1);
}

TEST(RemoteInput, OptionMappingPreservesOtherKeys)
{
  EXPECT_EQ(remoteInput::macOSKey(XK_Alt_L), XK_Mode_switch);
  EXPECT_EQ(remoteInput::macOSKey(XK_Alt_R), XK_ISO_Level3_Shift);
  for (uint32_t key : {XK_Control_L, XK_Super_L, XK_Super_R, XK_Shift_L,
                       XK_ISO_Level3_Shift, XK_a})
    EXPECT_EQ(remoteInput::macOSKey(key), key);
}
