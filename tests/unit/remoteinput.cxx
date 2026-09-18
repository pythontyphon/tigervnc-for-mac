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

TEST(RemoteInput, PacedWheelSpreadsStepsAndPreservesButtons)
{
  remoteInput::PacedWheel wheel;
  std::vector<uint16_t> events;
  auto send = [&](uint16_t mask) { events.push_back(mask); };
  wheel.add(1, -1, 12);
  for (int i = 0; i < 12; ++i) {
    ASSERT_FALSE(wheel.empty());
    events.clear();
    wheel.tick(5, send);
    EXPECT_EQ(events, (std::vector<uint16_t>{13, 5, 69, 5}));
  }
  EXPECT_TRUE(wheel.empty());
  events.clear();
  wheel.tick(0, send);
  EXPECT_TRUE(events.empty());
}

TEST(RemoteInput, PacedWheelBoundsBacklogAndReversesImmediately)
{
  remoteInput::PacedWheel wheel;
  wheel.add(INT_MIN, INT_MAX, 50);
  wheel.add(INT_MIN, INT_MAX, 50);
  int ticks = 0;
  while (!wheel.empty() && ticks < 100) {
    wheel.tick(0, [](uint16_t) {});
    ++ticks;
  }
  EXPECT_EQ(ticks, 24);
  wheel.add(0, 5, 12);
  wheel.add(0, -1, 2);
  std::vector<uint16_t> events;
  wheel.tick(0, [&](uint16_t mask) { events.push_back(mask); });
  EXPECT_EQ(events, (std::vector<uint16_t>{8, 0}));
  wheel.tick(0, [](uint16_t) {});
  EXPECT_TRUE(wheel.empty());
  wheel.add(1, 1, 12);
  wheel.clear();
  EXPECT_TRUE(wheel.empty());
}
