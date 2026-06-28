# Distance and direction from muzzle to target, computed by navigation group
float64 target_x     # horizontal distance in map XY plane (m)
float64 target_h     # height difference: target_z - muzzle_z (m), positive = target above
float64 azimuth      # map-frame bearing to target (rad), for control group to convert to IMU frame
bool    tf_ready     # true if map→muzzle TF chain is complete and valid
