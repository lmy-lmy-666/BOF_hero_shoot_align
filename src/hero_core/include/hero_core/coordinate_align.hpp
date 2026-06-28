#ifndef HERO_CORE__COORDINATE_ALIGN_HPP_
#define HERO_CORE__COORDINATE_ALIGN_HPP_

#include <cmath>
#include <string>

namespace hero_core
{

/** Result of yaw offset calibration between map frame and IMU world frame. */
struct AlignResult
{
  double yaw_offset = 0.0;  // map_yaw - imu_yaw, so gimbal_cmd = azimuth - offset
  bool valid = false;
  std::string error_msg;
};

/**
 * Calibrate yaw offset between map frame and IMU world frame.
 *
 * The map frame and IMU world frame share the same gravity-aligned Z axis,
 * but their Yaw (yaw) origins differ by an unknown rotation. This function
 * computes that offset from a pair of simultaneous measurements.
 *
 * @param map_yaw  chassis yaw in map frame (rad), from TF map→chassis
 * @param imu_yaw  gimbal yaw feedback in IMU world frame (rad)
 * @return AlignResult with yaw_offset = map_yaw - imu_yaw
 *
 * After calibration, the gimbal yaw command is:
 *     gimbal_yaw_cmd = azimuth - yaw_offset
 *
 * Usage:
 *   // Multiple samples are recommended; average the offsets for robustness.
 *   auto r = calibrate_yaw_offset(map_yaw, imu_yaw);
 *   if (r.valid) { ... }
 */
inline AlignResult calibrate_yaw_offset(double map_yaw, double imu_yaw)
{
  AlignResult result;
  result.yaw_offset = map_yaw - imu_yaw;
  result.valid = true;
  return result;
}

/**
 * Apply yaw offset to convert a map-frame azimuth into an IMU-frame
 * gimbal yaw command.
 *
 *   gimbal_cmd = azimuth - yaw_offset
 *
 * @param azimuth     target direction in map frame (rad)
 * @param yaw_offset  calibrated offset: map_yaw - imu_yaw (rad)
 * @return gimbal yaw command in IMU frame (rad)
 */
inline double apply_yaw_offset(double azimuth, double yaw_offset)
{
  return azimuth - yaw_offset;
}

}  // namespace hero_core

#endif  // HERO_CORE__COORDINATE_ALIGN_HPP_
