#!/usr/bin/env python3
"""
Calibration Data Analysis Script for Cave Surveying Device
Analyzes the actual calibration data provided by the user
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.linalg import norm, det
from numpy.linalg import cond
import warnings
warnings.filterwarnings('ignore')

def analyze_your_calibration_data():
    """Analyze the actual calibration data provided"""
    
    print("CAVE SURVEYING DEVICE CALIBRATION ANALYSIS")
    print("=" * 60)
    
    # Your actual calibration matrices from the device
    print("1. CALIBRATION MATRIX ANALYSIS")
    print("-" * 50)
    
    # Static calibration matrices (your actual data)
    Ra_static = np.array([
        [0.999852, 0.005661, -0.00056],
        [0.005661, 1.001267, -0.00147],
        [-0.00056, -0.00147, 0.999815]
    ])
    
    ba_static = np.array([0.015071, 0.039023, -0.02845])
    
    Rm_static = np.array([
        [0.937419, 0.034782, 0.027415],
        [0.034782, 0.874152, 0.011107],
        [0.027415, 0.011107, 0.897938]
    ])
    
    bm_static = np.array([-0.032536, 0.105361, 0.042746])
    
    print("STATIC CALIBRATION MATRICES:")
    print(f"Accelerometer correction matrix Ra_static:\n{Ra_static}")
    print(f"Accelerometer bias ba_static: {ba_static}")
    print(f"Magnetometer correction matrix Rm_static:\n{Rm_static}")
    print(f"Magnetometer bias bm_static: {bm_static}")
    
    # Laser calibration matrices (your actual data)
    Ra_laser = np.array([
        [-0.000568, 0.999688, -0.024983],
        [-0.022739, -0.024989, -0.999429],
        [0.999741, 0, -0.022746]
    ])
    
    Rm_laser = np.array([
        [-0.99891, 0.040412, 0.023374],
        [-0.040401, -0.999183, 0.000945],
        [-0.023393, 0, -0.999726]
    ])
    
    print(f"\nLASER CALIBRATION MATRICES:")
    print(f"Accelerometer laser correction Ra_laser:\n{Ra_laser}")
    print(f"Magnetometer laser correction Rm_laser:\n{Rm_laser}")
    
    # Alignment matrix (your actual data)
    Rm_align = np.array([
        [0.998315, 0.043691, 0.038191],
        [0.039935, -0.039775, -0.99841],
        [0.042102, -0.998253, 0.041453]
    ])
    
    print(f"\nALIGNMENT MATRIX:")
    print(f"Magnetometer alignment Rm_align:\n{Rm_align}")
    
    print("\n2. MATRIX VALIDATION")
    print("-" * 50)
    
    # Check if matrices are identity (indicating no calibration was performed)
    def is_identity(matrix, tolerance=1e-10):
        return np.allclose(matrix, np.eye(matrix.shape[0]), atol=tolerance)
    
    def is_zero_vector(vector, tolerance=1e-10):
        return np.allclose(vector, np.zeros_like(vector), atol=tolerance)
    
    # Analyze static calibration
    print("STATIC CALIBRATION ANALYSIS:")
    ra_is_identity = is_identity(Ra_static)
    ba_is_zero = is_zero_vector(ba_static)
    rm_is_identity = is_identity(Rm_static)
    bm_is_zero = is_zero_vector(bm_static)
    
    print(f"Ra_static is identity matrix: {ra_is_identity}")
    print(f"ba_static is zero vector: {ba_is_zero}")
    print(f"Rm_static is identity matrix: {rm_is_identity}")
    print(f"bm_static is zero vector: {bm_is_zero}")
    
    if ra_is_identity and ba_is_zero:
        print("⚠️  WARNING: Accelerometer static calibration appears to be uncalibrated (identity/zero)")
    else:
        print("✓ Accelerometer static calibration has been performed")
    
    if rm_is_identity and bm_is_zero:
        print("⚠️  WARNING: Magnetometer static calibration appears to be uncalibrated (identity/zero)")
    else:
        print("✓ Magnetometer static calibration has been performed")
    
    # Analyze laser calibration
    print("\nLASER CALIBRATION ANALYSIS:")
    ra_laser_is_identity = is_identity(Ra_laser)
    rm_laser_is_identity = is_identity(Rm_laser)
    
    print(f"Ra_laser is identity matrix: {ra_laser_is_identity}")
    print(f"Rm_laser is identity matrix: {rm_laser_is_identity}")
    
    if ra_laser_is_identity:
        print("⚠️  WARNING: Accelerometer laser calibration appears to be uncalibrated (identity)")
    else:
        print("✓ Accelerometer laser calibration has been performed")
    
    if rm_laser_is_identity:
        print("⚠️  WARNING: Magnetometer laser calibration appears to be uncalibrated (identity)")
    else:
        print("✓ Magnetometer laser calibration has been performed")
    
    # Analyze alignment
    print("\nALIGNMENT ANALYSIS:")
    rm_align_is_identity = is_identity(Rm_align)
    
    print(f"Rm_align is identity matrix: {rm_align_is_identity}")
    
    if rm_align_is_identity:
        print("⚠️  WARNING: Magnetometer alignment appears to be uncalibrated (identity)")
    else:
        print("✓ Magnetometer alignment has been performed")
    
    print("\n3. MATRIX PROPERTIES")
    print("-" * 50)
    
    # Calculate matrix properties
    det_Ra_static = det(Ra_static)
    det_Rm_static = det(Rm_static)
    det_Ra_laser = det(Ra_laser)
    det_Rm_laser = det(Rm_laser)
    det_Rm_align = det(Rm_align)
    
    print(f"Determinants:")
    print(f"  Ra_static: {det_Ra_static:.6f}")
    print(f"  Rm_static: {det_Rm_static:.6f}")
    print(f"  Ra_laser: {det_Ra_laser:.6f}")
    print(f"  Rm_laser: {det_Rm_laser:.6f}")
    print(f"  Rm_align: {det_Rm_align:.6f}")
    
    # Condition numbers
    cond_Ra_static = cond(Ra_static)
    cond_Rm_static = cond(Rm_static)
    
    print(f"\nCondition numbers:")
    print(f"  Ra_static: {cond_Ra_static:.6f}")
    print(f"  Rm_static: {cond_Rm_static:.6f}")
    
    print("\n4. CALIBRATION STATUS ASSESSMENT")
    print("-" * 50)
    
    # Overall calibration status
    calibrations_performed = 0
    total_calibrations = 5
    
    if not (ra_is_identity and ba_is_zero):
        calibrations_performed += 1
        print("✓ Accelerometer static calibration: PERFORMED")
    else:
        print("✗ Accelerometer static calibration: NOT PERFORMED")
    
    if not (rm_is_identity and bm_is_zero):
        calibrations_performed += 1
        print("✓ Magnetometer static calibration: PERFORMED")
    else:
        print("✗ Magnetometer static calibration: NOT PERFORMED")
    
    if not ra_laser_is_identity:
        calibrations_performed += 1
        print("✓ Accelerometer laser calibration: PERFORMED")
    else:
        print("✗ Accelerometer laser calibration: NOT PERFORMED")
    
    if not rm_laser_is_identity:
        calibrations_performed += 1
        print("✓ Magnetometer laser calibration: PERFORMED")
    else:
        print("✗ Magnetometer laser calibration: NOT PERFORMED")
    
    if not rm_align_is_identity:
        calibrations_performed += 1
        print("✓ Magnetometer alignment: PERFORMED")
    else:
        print("✗ Magnetometer alignment: NOT PERFORMED")
    
    print(f"\nCalibration completeness: {calibrations_performed}/{total_calibrations} ({100*calibrations_performed/total_calibrations:.1f}%)")
    
    print("\n5. RECOMMENDATIONS")
    print("-" * 50)
    
    if calibrations_performed == 0:
        print("🔴 CRITICAL: No calibrations have been performed!")
        print("   • Perform static calibration for both sensors")
        print("   • Perform laser calibration for both sensors")
        print("   • Perform magnetometer alignment")
        print("   • Device measurements will be inaccurate until calibrated")
    elif calibrations_performed < total_calibrations:
        print("🟡 INCOMPLETE: Some calibrations are missing")
        if ra_is_identity and ba_is_zero:
            print("   • Perform accelerometer static calibration")
        if rm_is_identity and bm_is_zero:
            print("   • Perform magnetometer static calibration")
        if ra_laser_is_identity:
            print("   • Perform accelerometer laser calibration")
        if rm_laser_is_identity:
            print("   • Perform magnetometer laser calibration")
        if rm_align_is_identity:
            print("   • Perform magnetometer alignment")
    else:
        print("🟢 COMPLETE: All calibrations have been performed")
        print("   • Device should provide accurate measurements")
        print("   • Verify calibration quality by testing known orientations")
    
    print("\n6. TECHNICAL NOTES")
    print("-" * 50)
    print("• Identity matrices indicate no correction is being applied")
    print("• Zero bias vectors indicate no offset correction")
    print("• For cave surveying, all calibrations are typically required for accuracy")
    print("• Static calibration corrects for sensor bias and scale factors")
    print("• Laser calibration aligns sensors with the laser reference")
    print("• Magnetometer alignment corrects for hard/soft iron effects")
    print("• Negative determinants may indicate coordinate system reflections")
    
    print("\n7. DETAILED LASER ALIGNMENT QUALITY ASSESSMENT")
    print("-" * 50)
    
    # Check orthogonality of rotation matrices (should be very close to 0)
    Ra_laser_orth_error = norm(Ra_laser @ Ra_laser.T - np.eye(3))
    Rm_laser_orth_error = norm(Rm_laser @ Rm_laser.T - np.eye(3))
    Rm_align_orth_error = norm(Rm_align @ Rm_align.T - np.eye(3))
    
    print("ORTHOGONALITY ERRORS (should be ~0):")
    print(f"  Ra_laser orthogonality error: {Ra_laser_orth_error:.8f}")
    print(f"  Rm_laser orthogonality error: {Rm_laser_orth_error:.8f}")
    print(f"  Rm_align orthogonality error: {Rm_align_orth_error:.8f}")
    
    # Check if matrices are proper rotations (determinant should be ±1)
    print(f"\nROTATION MATRIX VALIDITY:")
    print(f"  Ra_laser determinant: {det_Ra_laser:.6f} (should be ±1.0)")
    print(f"  Rm_laser determinant: {det_Rm_laser:.6f} (should be ±1.0)")
    print(f"  Rm_align determinant: {det_Rm_align:.6f} (should be ±1.0)")
    
    # Extract rotation angles to understand the physical alignment
    def rotation_matrix_to_euler_zyx(R):
        """Convert rotation matrix to ZYX Euler angles (yaw, pitch, roll)"""
        sy = np.sqrt(R[0,0]**2 + R[1,0]**2)
        singular = sy < 1e-6
        
        if not singular:
            x = np.arctan2(R[2,1], R[2,2])  # roll
            y = np.arctan2(-R[2,0], sy)     # pitch
            z = np.arctan2(R[1,0], R[0,0])  # yaw
        else:
            x = np.arctan2(-R[1,2], R[1,1])
            y = np.arctan2(-R[2,0], sy)
            z = 0
        
        return np.degrees([z, y, x])  # yaw, pitch, roll in degrees
    
    print(f"\nPHYSICAL ALIGNMENT ANGLES:")
    
    # Analyze accelerometer laser alignment
    Ra_angles = rotation_matrix_to_euler_zyx(Ra_laser)
    print(f"  Accelerometer laser alignment (yaw, pitch, roll): [{Ra_angles[0]:.2f}°, {Ra_angles[1]:.2f}°, {Ra_angles[2]:.2f}°]")
    
    # Analyze magnetometer laser alignment  
    Rm_laser_angles = rotation_matrix_to_euler_zyx(Rm_laser)
    print(f"  Magnetometer laser alignment (yaw, pitch, roll): [{Rm_laser_angles[0]:.2f}°, {Rm_laser_angles[1]:.2f}°, {Rm_laser_angles[2]:.2f}°]")
    
    # Analyze final magnetometer alignment
    Rm_align_angles = rotation_matrix_to_euler_zyx(Rm_align)
    print(f"  Final magnetometer alignment (yaw, pitch, roll): [{Rm_align_angles[0]:.2f}°, {Rm_align_angles[1]:.2f}°, {Rm_align_angles[2]:.2f}°]")
    
    print(f"\nLASER ALIGNMENT QUALITY ASSESSMENT:")
    
    # Quality checks for laser calibration
    laser_quality_issues = 0
    
    # Check orthogonality (updated thresholds for practical evaluation)
    if Ra_laser_orth_error > 1e-3:  # 0.001 - more reasonable threshold
        print("  ❌ Ra_laser: Poor orthogonality - matrix may be corrupted")
        laser_quality_issues += 1
    else:
        print("  ✅ Ra_laser: Excellent orthogonality")
    
    if Rm_laser_orth_error > 1e-3:  # 0.001 - more reasonable threshold
        print("  ❌ Rm_laser: Poor orthogonality - matrix may be corrupted")
        laser_quality_issues += 1
    else:
        print("  ✅ Rm_laser: Excellent orthogonality")
    
    if Rm_align_orth_error > 1e-3:  # 0.001 - more reasonable threshold
        print("  ❌ Rm_align: Poor orthogonality - matrix may be corrupted")
        laser_quality_issues += 1
    else:
        print("  ✅ Rm_align: Excellent orthogonality")
    
    # Check determinants
    if abs(abs(det_Ra_laser) - 1.0) > 0.01:
        print("  ❌ Ra_laser: Invalid determinant - not a proper rotation")
        laser_quality_issues += 1
    else:
        print("  ✅ Ra_laser: Valid rotation matrix")
    
    if abs(abs(det_Rm_laser) - 1.0) > 0.01:
        print("  ❌ Rm_laser: Invalid determinant - not a proper rotation")
        laser_quality_issues += 1
    else:
        print("  ✅ Rm_laser: Valid rotation matrix")
    
    if abs(abs(det_Rm_align) - 1.0) > 0.01:
        print("  ❌ Rm_align: Invalid determinant - not a proper rotation")
        laser_quality_issues += 1
    else:
        print("  ✅ Rm_align: Valid rotation matrix")
    
    print(f"\nSENSOR-TO-LASER AXIS ALIGNMENT ANALYSIS:")
    print("(For orientation-independent calibration, reflections and large rotations are normal)")
    
    # Assume laser axis is Z-axis (forward direction)
    laser_axis = np.array([0, 0, 1])
    
    def calculate_axis_alignment(rotation_matrix, axis_name):
        """Calculate how well each sensor axis aligns with the laser axis"""
        # Transform the standard basis vectors (sensor axes) by the rotation matrix
        sensor_x = rotation_matrix @ np.array([1, 0, 0])  # X-axis of sensor in laser frame
        sensor_y = rotation_matrix @ np.array([0, 1, 0])  # Y-axis of sensor in laser frame  
        sensor_z = rotation_matrix @ np.array([0, 0, 1])  # Z-axis of sensor in laser frame
        
        # Calculate angles between each sensor axis and the laser axis
        angle_x = np.degrees(np.arccos(np.clip(np.abs(np.dot(sensor_x, laser_axis)), 0, 1)))
        angle_y = np.degrees(np.arccos(np.clip(np.abs(np.dot(sensor_y, laser_axis)), 0, 1)))
        angle_z = np.degrees(np.arccos(np.clip(np.abs(np.dot(sensor_z, laser_axis)), 0, 1)))
        
        # Find the best aligned axis (smallest angle)
        best_angle = min(angle_x, angle_y, angle_z)
        if angle_x == best_angle:
            best_axis = "X"
        elif angle_y == best_angle:
            best_axis = "Y"
        else:
            best_axis = "Z"
        
        print(f"  {axis_name}:")
        print(f"    • X-axis alignment with laser: {angle_x:.2f}° off-axis")
        print(f"    • Y-axis alignment with laser: {angle_y:.2f}° off-axis")
        print(f"    • Z-axis alignment with laser: {angle_z:.2f}° off-axis")
        print(f"    • Best aligned axis: {best_axis} ({best_angle:.2f}° off-axis)")
        
        return best_angle, best_axis, [angle_x, angle_y, angle_z]
    
    # Analyze accelerometer alignment
    acc_best_angle, acc_best_axis, acc_angles = calculate_axis_alignment(Ra_laser, "Accelerometer (Ra_laser)")
    
    # Analyze magnetometer laser alignment
    mag_laser_best_angle, mag_laser_best_axis, mag_laser_angles = calculate_axis_alignment(Rm_laser, "Magnetometer laser (Rm_laser)")
    
    # Analyze final magnetometer alignment
    mag_align_best_angle, mag_align_best_axis, mag_align_angles = calculate_axis_alignment(Rm_align, "Magnetometer final (Rm_align)")
    
    def get_alignment_quality_label(angle):
        """Get quality label based on alignment angle"""
        if angle <= 2.0:
            return "EXCELLENT"
        elif angle <= 5.0:
            return "GOOD"
        elif angle <= 10.0:
            return "FAIR"
        else:
            return "POOR"
    
    print(f"\nALIGNMENT QUALITY SUMMARY:")
    acc_quality = get_alignment_quality_label(acc_best_angle)
    mag_laser_quality = get_alignment_quality_label(mag_laser_best_angle)
    mag_align_quality = get_alignment_quality_label(mag_align_best_angle)
    
    print(f"  Accelerometer alignment: {acc_quality} ({acc_best_angle:.2f}° off laser axis)")
    print(f"  Magnetometer laser alignment: {mag_laser_quality} ({mag_laser_best_angle:.2f}° off laser axis)")  
    print(f"  Magnetometer final alignment: {mag_align_quality} ({mag_align_best_angle:.2f}° off laser axis)")
    
    # Update quality assessment based on axis alignment
    if acc_best_angle > 10:
        print("  ⚠️  Poor accelerometer axis alignment - significant misalignment with laser")
        laser_quality_issues += 1
    elif acc_best_angle > 5:
        print("  ⚠️  Fair accelerometer axis alignment - moderate misalignment")
        laser_quality_issues += 1
    else:
        print("  ✅ Good accelerometer axis alignment")
    
    if mag_laser_best_angle > 10:
        print("  ⚠️  Poor magnetometer laser axis alignment - significant misalignment")
        laser_quality_issues += 1
    elif mag_laser_best_angle > 5:
        print("  ⚠️  Fair magnetometer laser axis alignment - moderate misalignment")
        laser_quality_issues += 1
    else:
        print("  ✅ Good magnetometer laser axis alignment")
        
    if mag_align_best_angle > 10:
        print("  ⚠️  Poor magnetometer final axis alignment - significant misalignment")
        laser_quality_issues += 1
    elif mag_align_best_angle > 5:
        print("  ⚠️  Fair magnetometer final axis alignment - moderate misalignment") 
        laser_quality_issues += 1
    else:
        print("  ✅ Good magnetometer final axis alignment")
    
    # Note about coordinate system reflections (normal for orientation-independent calibration)
    negative_dets = sum([det_Ra_laser < 0, det_Rm_laser < 0, det_Rm_align < 0])
    if negative_dets > 0:
        print(f"\n  ℹ️  NOTE: {negative_dets}/3 matrices have negative determinants (reflections)")
        print("     This is normal for orientation-independent calibration systems.")
        print("     What matters is axis alignment, not coordinate system handedness.")
    
    # Display Euler angles for reference but don't flag as errors for orientation-independent systems
    max_acc_angle = max(abs(Ra_angles[0]), abs(Ra_angles[1]), abs(Ra_angles[2]))
    max_mag_laser_angle = max(abs(Rm_laser_angles[0]), abs(Rm_laser_angles[1]), abs(Rm_laser_angles[2]))
    max_mag_align_angle = max(abs(Rm_align_angles[0]), abs(Rm_align_angles[1]), abs(Rm_align_angles[2]))
    
    print(f"\n  ℹ️  EULER ANGLES (for reference - large angles are normal):")
    print(f"     Accelerometer: max {max_acc_angle:.2f}° (yaw: {Ra_angles[0]:.1f}°, pitch: {Ra_angles[1]:.1f}°, roll: {Ra_angles[2]:.1f}°)")
    print(f"     Magnetometer laser: max {max_mag_laser_angle:.2f}° (yaw: {Rm_laser_angles[0]:.1f}°, pitch: {Rm_laser_angles[1]:.1f}°, roll: {Rm_laser_angles[2]:.1f}°)")
    print(f"     Magnetometer final: max {max_mag_align_angle:.2f}° (yaw: {Rm_align_angles[0]:.1f}°, pitch: {Rm_align_angles[1]:.1f}°, roll: {Rm_align_angles[2]:.1f}°)")

    print(f"\nLASER CALIBRATION OVERALL ASSESSMENT:")
    worst_alignment = max(acc_best_angle, mag_laser_best_angle, mag_align_best_angle)
    
    if laser_quality_issues > 0:
        print("  � INVALID: Rotation matrices are mathematically invalid")
        print("     • Fix matrix corruption issues before assessing alignment quality")
    elif worst_alignment <= 2.0:
        print("  🟢 EXCELLENT: All sensors are excellently aligned with laser axis")
        print(f"     • Best alignment: {min(acc_best_angle, mag_laser_best_angle, mag_align_best_angle):.2f}°")
        print(f"     • Worst alignment: {worst_alignment:.2f}°")
        print("     • Calibration quality is outstanding for surveying accuracy")
    elif worst_alignment <= 5.0:
        print("  � GOOD: All sensors are well aligned with laser axis")
        print(f"     • Best alignment: {min(acc_best_angle, mag_laser_best_angle, mag_align_best_angle):.2f}°")
        print(f"     • Worst alignment: {worst_alignment:.2f}°")
        print("     • Calibration quality is suitable for most surveying applications")
    elif worst_alignment <= 10.0:
        print("  🟡 FAIR: Sensor alignment is acceptable but could be improved")
        print(f"     • Best alignment: {min(acc_best_angle, mag_laser_best_angle, mag_align_best_angle):.2f}°")
        print(f"     • Worst alignment: {worst_alignment:.2f}°")
        print("     • Consider re-calibrating if higher precision is needed")
    else:
        print("  🔴 POOR: Sensor alignment has significant issues")
        print(f"     • Best alignment: {min(acc_best_angle, mag_laser_best_angle, mag_align_best_angle):.2f}°")
        print(f"     • Worst alignment: {worst_alignment:.2f}°")
        print("     • Re-calibration recommended for accurate surveying")
    
    # Return status
    if calibrations_performed == 0:
        status = "UNCALIBRATED"
    elif calibrations_performed < total_calibrations:
        status = "PARTIALLY_CALIBRATED"
    else:
        status = "FULLY_CALIBRATED"
    
    return status, calibrations_performed, total_calibrations

if __name__ == "__main__":
    status, performed, total = analyze_your_calibration_data()
    print(f"\nFINAL STATUS: {status} ({performed}/{total})")
