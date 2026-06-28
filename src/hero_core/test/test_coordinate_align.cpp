#include <gtest/gtest.h>
#include <cmath>

#include "hero_core/coordinate_align.hpp"

using hero_core::calibrate_yaw_offset;
using hero_core::apply_yaw_offset;

constexpr double TOL = 1e-9;
constexpr double PI = 3.141592653589793;

// --- Zero offset when frames are aligned ---
TEST(CoordinateAlign, ZeroOffset)
{
  auto r = calibrate_yaw_offset(0.5, 0.5);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.yaw_offset, 0.0, TOL);
}

// --- Positive offset ---
TEST(CoordinateAlign, PositiveOffset)
{
  // map yaw = 1.0 rad, imu yaw = 0.5 rad → offset = +0.5
  auto r = calibrate_yaw_offset(1.0, 0.5);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.yaw_offset, 0.5, TOL);
}

// --- Negative offset ---
TEST(CoordinateAlign, NegativeOffset)
{
  auto r = calibrate_yaw_offset(0.2, 0.9);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.yaw_offset, -0.7, TOL);
}

// --- Apply offset: simple case ---
TEST(CoordinateAlign, ApplyOffset)
{
  double cmd = apply_yaw_offset(0.8, 0.3);  // azimuth=0.8, offset=0.3
  EXPECT_NEAR(cmd, 0.5, TOL);
}

// --- Apply offset: negative offset ---
TEST(CoordinateAlign, ApplyOffsetNegative)
{
  double cmd = apply_yaw_offset(-0.5, -0.2);
  EXPECT_NEAR(cmd, -0.3, TOL);
}

// --- Round-trip: calibrate then apply should recover original ---
TEST(CoordinateAlign, RoundTrip)
{
  double map_yaw = 1.2;
  double imu_yaw = -0.4;

  auto r = calibrate_yaw_offset(map_yaw, imu_yaw);
  ASSERT_TRUE(r.valid);

  // Given map_yaw as azimuth, applying the offset should give back imu_yaw
  double cmd = apply_yaw_offset(map_yaw, r.yaw_offset);
  EXPECT_NEAR(cmd, imu_yaw, TOL);
}
