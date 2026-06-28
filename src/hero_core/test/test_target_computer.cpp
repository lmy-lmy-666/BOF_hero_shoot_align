#include <gtest/gtest.h>
#include <cmath>

#include "hero_core/target_computer.hpp"

using hero_core::compute_target;

constexpr double TOL = 1e-9;

// --- Basic horizontal target (same height) ---
TEST(TargetComputer, HorizontalTargetSameHeight)
{
  // Muzzle at origin, target 10m east, same height
  auto r = compute_target(0.0, 0.0, 1.0, 10.0, 0.0, 1.0);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.target_x, 10.0, TOL);
  EXPECT_NEAR(r.target_h, 0.0, TOL);
  EXPECT_NEAR(r.azimuth, 0.0, TOL);  // due east
}

// --- Target above ---
TEST(TargetComputer, TargetAbove)
{
  auto r = compute_target(0.0, 0.0, 0.5, 5.0, 0.0, 1.5);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.target_x, 5.0, TOL);
  EXPECT_NEAR(r.target_h, 1.0, TOL);  // 1.5 - 0.5
}

// --- Target below ---
TEST(TargetComputer, TargetBelow)
{
  auto r = compute_target(0.0, 0.0, 2.0, 8.0, 0.0, 0.5);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.target_x, 8.0, TOL);
  EXPECT_NEAR(r.target_h, -1.5, TOL);  // 0.5 - 2.0
}

// --- Diagonal direction ---
TEST(TargetComputer, DiagonalNorthEast)
{
  auto r = compute_target(0.0, 0.0, 0.0, 3.0, 4.0, 0.0);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.target_x, 5.0, TOL);  // sqrt(9+16)
  EXPECT_NEAR(r.target_h, 0.0, TOL);
  EXPECT_NEAR(r.azimuth, std::atan2(4.0, 3.0), TOL);
}

// --- Negative azimuth (south-east) ---
TEST(TargetComputer, SouthEastQuadrant)
{
  auto r = compute_target(0.0, 0.0, 0.0, 10.0, -5.0, 0.0);
  ASSERT_TRUE(r.valid);
  EXPECT_NEAR(r.azimuth, std::atan2(-5.0, 10.0), TOL);
  EXPECT_LT(r.azimuth, 0.0);  // negative = clockwise from +X
}

// --- RM2026 realistic scenario ---
TEST(TargetComputer, Rm2026Realistic)
{
  // muzzle near center of field, base at far corner
  double muzzle_x = 14.0, muzzle_y = 9.0, muzzle_z = 0.9;
  double base_x = 23.125, base_y = 1.51, base_z = 0.84;

  auto r = compute_target(muzzle_x, muzzle_y, muzzle_z, base_x, base_y, base_z);

  ASSERT_TRUE(r.valid);

  double expected_x = std::sqrt(std::pow(23.125 - 14.0, 2) + std::pow(1.51 - 9.0, 2));
  double expected_h = 0.84 - 0.9;

  EXPECT_NEAR(r.target_x, expected_x, TOL);
  EXPECT_NEAR(r.target_h, expected_h, TOL);
  EXPECT_LT(r.azimuth, 0.0);  // target is south-east-ish
}
