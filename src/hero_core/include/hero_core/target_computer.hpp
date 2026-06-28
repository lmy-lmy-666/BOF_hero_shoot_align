#ifndef HERO_CORE__TARGET_COMPUTER_HPP_
#define HERO_CORE__TARGET_COMPUTER_HPP_

#include <cmath>
#include <string>

namespace hero_core
{

/** Result of computing target distance and direction from muzzle to target. */
struct TargetResult
{
  double target_x = 0.0;   // horizontal distance in map XY plane (m)
  double target_h = 0.0;   // height difference: target_z - muzzle_z (m)
  double azimuth = 0.0;    // map-frame azimuth angle to target (rad)
  bool valid = false;      // true if computation succeeded
  std::string error_msg;   // description of failure (empty if valid)
};

/**
 * Compute horizontal distance, height difference, and azimuth from a
 * source point to a target point in a 3D map frame.
 *
 * @param src_x   source X in map frame (m)
 * @param src_y   source Y in map frame (m)
 * @param src_z   source Z in map frame (m)
 * @param tgt_x   target X in map frame (m)
 * @param tgt_y   target Y in map frame (m)
 * @param tgt_z   target Z in map frame (m)
 * @return TargetResult  containing target_x, target_h, azimuth, and valid flag
 *
 * Usage:
 *   auto r = compute_target(10.0, 5.0, 0.9, 23.125, 1.51, 0.84);
 *   if (r.valid) {
 *       // r.target_x → horizontal range (m)
 *       // r.target_h → height difference (m)
 *       // r.azimuth  → map-frame bearing (rad)
 *   }
 */
inline TargetResult compute_target(
  double src_x, double src_y, double src_z,
  double tgt_x, double tgt_y, double tgt_z)
{
  TargetResult result;

  double dx = tgt_x - src_x;
  double dy = tgt_y - src_y;

  result.target_x = std::sqrt(dx * dx + dy * dy);
  result.target_h = tgt_z - src_z;
  result.azimuth = std::atan2(dy, dx);
  result.valid = true;

  return result;
}

}  // namespace hero_core

#endif  // HERO_CORE__TARGET_COMPUTER_HPP_
